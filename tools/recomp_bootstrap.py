#!/usr/bin/env python3
"""Emit and verify Crash Bash's resident recompilation substrate from retail bytes."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "scratch/bin/crashbash/SCUS_945.70"
IDENTITY = ROOT / "titles/crashbash/executable.json"
SEEDS = ROOT / "game/recomp_seeds.json"
CONFIG = ROOT / "game/core/game_config.cpp"
GENERATED = ROOT / "generated"
MEASUREMENT_FILE = GENERATED / ".recomp.measurement.json"
SCRATCH = ROOT / "scratch/raw"


class Refused(RuntimeError):
    """Required evidence could not be produced."""


class Mismatch(RuntimeError):
    """Measured bytes disagree with a shipping input."""


def psxport_dir() -> Path:
    return Path(os.environ.get("PSXPORT_DIR", ROOT / "external/psxport")).resolve()


def recompiler_sources() -> list[Path]:
    directory = psxport_dir() / "tools/recomp"
    return [directory / name for name in ("emit.py", "decode.py", "psexe.py")]


def identity() -> dict:
    try:
        return json.loads(IDENTITY.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise Refused(f"cannot read target identity {IDENTITY}: {error}") from error


def require_retail(path: Path = EXE) -> dict:
    expected = identity()
    try:
        data = path.read_bytes()
    except OSError as error:
        raise Refused(f"cannot read retail executable {path}: {error}") from error
    digest = hashlib.sha256(data).hexdigest()
    if len(data) != expected["file_size"] or digest != expected["sha256"]:
        raise Refused(
            f"{path} is not verified SCUS_945.70: {len(data)} bytes sha256 {digest}; "
            f"expected {expected['file_size']} bytes sha256 {expected['sha256']}"
        )
    return expected


def invoke_emitter(
    output: Path, seeds: Path = SEEDS, shards: str = "8"
) -> subprocess.CompletedProcess[str]:
    emitter = recompiler_sources()[0]
    output.parent.mkdir(parents=True, exist_ok=True)
    environment = dict(os.environ)
    environment.update(PYTHONDONTWRITEBYTECODE="1", PSXPORT_SHARDS=shards)
    environment.pop("PSXPORT_USE_GHIDRA", None)
    return subprocess.run(
        [
            sys.executable,
            "-B",
            str(emitter),
            str(EXE),
            str(output),
            "--seeds",
            str(seeds),
        ],
        cwd=ROOT,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )


def generated_measurement(
    directory: Path, output: str, measured: dict[str, int]
) -> tuple[int, int, str]:
    count = re.search(r"\[func\] functions: (\d+) seeds -> (\d+) recompiled", output)
    if count is None:
        raise Refused("emitter returned no seed/function denominator")
    roots, functions = map(int, count.groups())
    if roots == 0 or functions < roots:
        raise Refused(
            f"invalid discovery denominator: {roots} roots -> {functions} functions"
        )

    try:
        declarations = (directory / "rec_decls.h").read_text(encoding="utf-8")
        table = (directory / "overlay_table.c").read_text(encoding="utf-8")
        header = (directory / "overlay_table.h").read_text(encoding="utf-8")
        manifest = (directory / "rec_sources.cmake").read_text(encoding="utf-8")
        version = (directory / ".recomp_version").read_text(encoding="utf-8").strip()
    except OSError as error:
        raise Refused(
            f"emitter omitted a required generated interface: {error}"
        ) from error

    required = (
        f"void func_{measured['crt0']:08X}(Core*);",
        f"void func_{measured['libcInit']:08X}(Core*);",
        f"void func_{measured['gameMain']:08X}(Core*);",
        "void main_dispatch(Core* c, uint32_t addr);",
        "void shard_set_override(uint32_t addr, OverrideFn fn);",
    )
    missing = [symbol for symbol in required if symbol not in declarations]
    if missing:
        raise Mismatch("generated declarations omit " + ", ".join(missing))
    if "const int g_rec_overlay_count = 0;" not in table:
        raise Mismatch(
            "resident-only emission did not produce a zero-entry overlay table"
        )
    for source in ("overlay_table.c", "shard_0.c", "shard_disp.c"):
        if source not in manifest:
            raise Mismatch(f"generated source manifest omits {source}")
    expected = require_retail()["header"]
    lo = int(expected["text_address"], 0) & 0x1FFFFFFF
    hi = (int(expected["text_address"], 0) + int(expected["text_size"], 0)) & 0x1FFFFFFF
    if (
        f"#define REC_MAIN_LO 0x{lo:08X}u" not in header
        or f"#define REC_MAIN_HI 0x{hi:08X}u" not in header
    ):
        raise Mismatch(f"generated resident range is not [0x{lo:08X},0x{hi:08X})")
    if not version:
        raise Refused("emitter wrote an empty recompiler version stamp")
    return roots, functions, version


def emit_and_measure(
    directory: Path, measured: dict[str, int], seeds: Path = SEEDS
) -> tuple[int, int, str]:
    result = invoke_emitter(directory / "main.c", seeds)
    if result.returncode != 0:
        raise Refused(f"emitter exited {result.returncode}:\n{result.stdout.rstrip()}")
    return generated_measurement(directory, result.stdout, measured)


def direct_jals(data: bytes, entry: int, load: int) -> list[int]:
    offset = 0x800 + entry - load
    calls: list[int] = []
    for index in range(96):
        at = offset + index * 4
        if at + 4 > len(data):
            raise Refused(f"crt0 scan left the image after {index} instruction(s)")
        word = int.from_bytes(data[at : at + 4], "little")
        if word & 0xFC00003F == 0x0000000D:
            break
        if word >> 26 == 3:
            pc = entry + index * 4
            calls.append(((pc + 4) & 0xF0000000) | ((word & 0x03FFFFFF) << 2))
    else:
        raise Refused("crt0 scan did not reach break within 96 instructions")
    if len(calls) != 2:
        raise Refused(
            f"crt0 has {len(calls)} direct jal(s) before break, expected exactly 2"
        )
    return calls


def crt0_measurement(extractor: Path) -> dict[str, int]:
    result = subprocess.run(
        [str(extractor), str(EXE)],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise Refused(
            f"crt0 extractor exited {result.returncode}:\n{result.stdout.rstrip()}"
        )
    fields: dict[str, int] = {}
    for name in (
        "bssZeroLo",
        "bssZeroHi",
        "stackTopBase",
        "stackTopBase2",
        "heapBase",
        "gp",
        "libcInit",
    ):
        match = re.search(
            rf"^\s*{name}\s+(0x[0-9A-Fa-f]+)$", result.stdout, re.MULTILINE
        )
        if match is None:
            raise Refused(f"crt0 extractor output omitted {name}")
        fields[name] = int(match.group(1), 16)
    bias = re.search(r"^\s*stackBias\s+(-?\d+)$", result.stdout, re.MULTILINE)
    if bias is None:
        raise Refused("crt0 extractor output omitted stackBias")
    fields["stackBias"] = int(bias.group(1))
    fields["heapSizePtr"] = 0
    fields["heapBasePtr"] = 0

    target = require_retail()
    entry = int(target["header"]["entry"], 0)
    load = int(target["header"]["text_address"], 0)
    calls = direct_jals(EXE.read_bytes(), entry, load)
    if calls[0] != fields["libcInit"]:
        raise Mismatch(
            f"crt0 extractor libcInit 0x{fields['libcInit']:08X} disagrees with first jal 0x{calls[0]:08X}"
        )
    fields["gameMain"] = calls[1]
    fields["crt0"] = entry
    return fields


CONSTANT_FIELDS = {
    "kCrt0BssZeroLo": "bssZeroLo",
    "kCrt0BssZeroHi": "bssZeroHi",
    "kCrt0StackTopBase": "stackTopBase",
    "kCrt0StackTopBase2": "stackTopBase2",
    "kCrt0HeapBase": "heapBase",
    "kCrt0HeapSizePtr": "heapSizePtr",
    "kCrt0HeapBasePtr": "heapBasePtr",
    "kCrt0Gp": "gp",
    "kCrt0LibcInit": "libcInit",
    "kCrt0GameMain": "gameMain",
    "kCrt0Entry": "crt0",
    "kCrt0StackBias": "stackBias",
}


def verify_config(measured: dict[str, int], source: Path = CONFIG) -> None:
    try:
        text = source.read_text(encoding="utf-8")
    except OSError as error:
        raise Refused(f"cannot read shipping config {source}: {error}") from error
    for constant, field in CONSTANT_FIELDS.items():
        match = re.search(
            rf"static constexpr (?:u?int32_t) {constant} = (0x[0-9A-Fa-f]+|\d+)u?;",
            text,
        )
        if match is None:
            raise Refused(f"shipping config omits literal {constant}")
        shipped = int(match.group(1), 0)
        if shipped != measured[field]:
            raise Mismatch(
                f"{constant} ships 0x{shipped & 0xFFFFFFFF:08X}, measured {field} is "
                f"0x{measured[field] & 0xFFFFFFFF:08X}"
            )
        if field != "stackBias":
            binding = rf"\.{field}\s*=\s*{constant}\s*,"
            if re.search(binding, text) is None:
                raise Mismatch(f"GameConfig::{field} is not bound to {constant}")
    if re.search(r"\.stackBias\s*=\s*\{1,\s*kCrt0StackBias\}\s*,", text) is None:
        raise Mismatch("GameConfig::stackBias does not declare kCrt0StackBias")


def input_hash() -> str:
    digest = hashlib.sha256()
    for path in [EXE, SEEDS, *recompiler_sources()]:
        if not path.is_file():
            raise Refused(f"required recomp input is absent: {path}")
        digest.update(path.name.encode())
        digest.update(path.read_bytes())
    return digest.hexdigest()


def generated_complete() -> bool:
    manifest = GENERATED / "rec_sources.cmake"
    if not manifest.is_file():
        return False
    names = re.findall(
        r"^\s*(\S+\.c)\s*$", manifest.read_text(encoding="utf-8"), re.MULTILINE
    )
    return bool(names) and all((GENERATED / name).is_file() for name in names)


def ensure(extractor: Path) -> tuple[int, int, str]:
    require_retail()
    measured = crt0_measurement(extractor)
    verify_config(measured)
    emitter = recompiler_sources()[0]
    version_match = re.search(
        r'^RECOMP_VERSION\s*=\s*"([^"]+)"', emitter.read_text(), re.MULTILINE
    )
    if version_match is None:
        raise Refused(f"cannot read RECOMP_VERSION from {emitter}")
    version = version_match.group(1)
    wanted = f"{version}:{input_hash()}"
    hash_file = GENERATED / ".recomp.hash"
    if (
        os.environ.get("PSXPORT_FORCE_RECOMP", "") in ("", "0")
        and hash_file.is_file()
        and hash_file.read_text().strip() == wanted
        and generated_complete()
        and MEASUREMENT_FILE.is_file()
    ):
        try:
            prior = json.loads(MEASUREMENT_FILE.read_text(encoding="utf-8"))
            prior_output = (
                f"[func] functions: {int(prior['roots'])} seeds -> "
                f"{int(prior['functions'])} recompiled"
            )
        except (KeyError, OSError, ValueError, json.JSONDecodeError) as error:
            raise Refused(f"cannot read prior recomp measurement: {error}") from error
        return generated_measurement(GENERATED, prior_output, measured)

    result = invoke_emitter(GENERATED / "main.c")
    if result.returncode != 0:
        raise Refused(f"emitter exited {result.returncode}:\n{result.stdout.rstrip()}")
    outcome = generated_measurement(GENERATED, result.stdout, measured)
    hash_file.write_text(wanted + "\n", encoding="utf-8")
    MEASUREMENT_FILE.write_text(
        json.dumps(
            {"roots": outcome[0], "functions": outcome[1], "version": outcome[2]},
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )
    return outcome


def check(extractor: Path) -> tuple[int, int, str]:
    require_retail()
    measured = crt0_measurement(extractor)
    verify_config(measured)
    SCRATCH.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix="crashbash-recomp-", dir=SCRATCH
    ) as temporary:
        return emit_and_measure(Path(temporary), measured)


def selftest(extractor: Path) -> bool:
    roots, functions, version = check(extractor)
    print(
        f"PASS positive: {roots} retail-binary roots -> {functions} resident functions; version {version}"
    )
    measured = crt0_measurement(extractor)
    passed = 1
    with tempfile.TemporaryDirectory(
        prefix="crashbash-negative-", dir=SCRATCH
    ) as temporary:
        directory = Path(temporary)
        changed = directory / "game_config.cpp"
        text = CONFIG.read_text(encoding="utf-8").replace(
            "0x8006E9F0u", "0x8006E9F4u", 1
        )
        changed.write_text(text, encoding="utf-8")
        try:
            verify_config(measured, changed)
        except Mismatch:
            passed += 1
            print("PASS negative: a changed shipping BSS bound is rejected")
        else:
            print("FAIL negative: changed shipping BSS bound passed", file=sys.stderr)

        bad_seeds = directory / "outside-text.json"
        bad_seeds.write_text('{"main": ["0x90000000"]}\n', encoding="utf-8")
        result = invoke_emitter(directory / "bad/main.c", bad_seeds, "1")
        if (
            result.returncode != 0
            and "seed(s) outside the module text" in result.stdout
        ):
            passed += 1
            print("PASS negative: an out-of-text explicit seed is refused")
        else:
            print("FAIL negative: out-of-text seed was not bounded", file=sys.stderr)

        wrong = directory / "SCUS_945.70"
        data = bytearray(EXE.read_bytes())
        data[-1] ^= 1
        wrong.write_bytes(data)
        try:
            require_retail(wrong)
        except Refused:
            passed += 1
            print("PASS negative: a one-byte executable mutation is refused")
        else:
            print("FAIL negative: mutated executable passed identity", file=sys.stderr)
    print(f"SELFTEST {passed}/4")
    return passed == 4


def default_extractor() -> Path:
    candidates = (
        ROOT / "scratch/build-clang/psxport_build/tools/crt0_extract",
        ROOT / "scratch/build/psxport_build/tools/crt0_extract",
    )
    for path in candidates:
        if path.is_file():
            return path
    return candidates[0]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument(
        "--ensure", action="store_true", help="refresh generated/ when inputs changed"
    )
    action.add_argument(
        "--check",
        action="store_true",
        help="emit in scratch and verify the shipping seam",
    )
    action.add_argument(
        "--selftest", action="store_true", help="run positive and forced-negative cases"
    )
    parser.add_argument("--crt0-extract", type=Path, default=default_extractor())
    args = parser.parse_args()
    try:
        if args.selftest:
            return 0 if selftest(args.crt0_extract) else 1
        outcome = ensure(args.crt0_extract) if args.ensure else check(args.crt0_extract)
        print(
            f"PASS: {outcome[0]} retail-binary roots -> {outcome[1]} resident functions; "
            f"crt0/InitHeap/gameMain present; version {outcome[2]}"
        )
        return 0
    except Mismatch as error:
        print(f"MISMATCH: {error}", file=sys.stderr)
        return 1
    except (OSError, Refused) as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
