#!/usr/bin/env python3
"""Verify Crash Bash reaches its deterministic nested-MENU entry boundary."""

from __future__ import annotations

import argparse
import io
import os
import queue
import re
import subprocess
import sys
import tempfile
import threading
import time
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_PORT = ROOT / "scratch/bin/crashbash_port"
DEFAULT_EXECUTABLE = ROOT / "scratch/bin/crashbash/SCUS_945.70"
DEFAULT_LOG = ROOT / "scratch/logs/verify-boot.log"
GENERATED_IDENTITY = ROOT / "generated/.recomp_identity"

LOAD_START = "load file start"
LOAD_COMPLETE = "done loading"
EMPTY_PRIMS = "empty prims"
MENU_ENTRY_ADDRESS = "800B5244"
MENU_ENTRY_CALLER_RA = "8001E7C0"
MENU_ENTRY = re.compile(
    rf"\[crashbash-boundary\]\s+MENU entry addr={MENU_ENTRY_ADDRESS}\s+"
    rf"ra={MENU_ENTRY_CALLER_RA}\b",
    re.IGNORECASE,
)
FORBIDDEN = (
    re.compile(r"\[watchdog\]\s+(?:STUCK|INTERRUPT)\b", re.IGNORECASE),
    re.compile(r"\bfatal\b", re.IGNORECASE),
    re.compile(r"\[recomp-MISS", re.IGNORECASE),
    re.compile(r"Segmentation fault", re.IGNORECASE),
    re.compile(r"CD timeout:", re.IGNORECASE),
    re.compile(r"VSync:\s*timeout", re.IGNORECASE),
    re.compile(r"Cant find CRASHBSH\.DAT", re.IGNORECASE),
)


class Refused(RuntimeError):
    """The runtime evidence did not establish the declared boundary."""

    def __init__(self, message: str, *, output: str = "") -> None:
        super().__init__(message)
        self.output = output


@dataclass(frozen=True)
class Verdict:
    lines: int
    load_starts: int
    load_completions: int
    menu_entry_hits: int


def expected_substrate_identity() -> str:
    try:
        identity = GENERATED_IDENTITY.read_text(encoding="utf-8").strip()
    except OSError as error:
        raise Refused(
            f"compiled-substrate identity is unavailable at {GENERATED_IDENTITY}: {error}"
        ) from error
    if not re.fullmatch(r"recomp-[0-9]{4}-[0-9]{2}-[0-9]{2}\.[0-9]+-[0-9a-f]{64}", identity):
        raise Refused(f"malformed compiled-substrate identity: {identity!r}")
    return identity


def _matching_lines(lines: list[str], pattern: str | re.Pattern[str]) -> list[int]:
    if isinstance(pattern, str):
        return [index for index, line in enumerate(lines) if line == pattern]
    return [index for index, line in enumerate(lines) if pattern.search(line)]


def judge(text: str, substrate_identity: str) -> Verdict:
    lines = [line.strip() for line in text.splitlines()]
    forbidden = [
        (pattern.pattern, index + 1)
        for pattern in FORBIDDEN
        for index, line in enumerate(lines)
        if pattern.search(line)
    ]
    if forbidden:
        details = ", ".join(
            f"{pattern!r} at line {line}" for pattern, line in forbidden
        )
        raise Refused(f"forbidden runtime failure: {details}")

    starts = _matching_lines(lines, LOAD_START)
    completions = _matching_lines(lines, LOAD_COMPLETE)
    menu_entries = _matching_lines(lines, MENU_ENTRY)
    empty_prims = _matching_lines(lines, EMPTY_PRIMS)
    identity_line = f"[recomp] generated substrate identity: {substrate_identity}"
    identities = _matching_lines(lines, identity_line)
    counts = (
        ("matching compiled-substrate identity", len(identities), 1),
        (LOAD_START, len(starts), 2),
        (LOAD_COMPLETE, len(completions), 2),
        ("matching MENU entry", len(menu_entries), 1),
        (EMPTY_PRIMS, len(empty_prims), 1),
    )
    mismatched = [
        f"{label}={actual}/{expected}"
        for label, actual, expected in counts
        if actual != expected
    ]
    if mismatched:
        raise Refused("boundary denominator mismatch: " + ", ".join(mismatched))

    ordered = (
        identities[0],
        starts[0],
        completions[0],
        starts[1],
        completions[1],
        empty_prims[0],
        menu_entries[0],
    )
    if ordered != tuple(sorted(ordered)) or len(set(ordered)) != len(ordered):
        raise Refused(
            "causal order mismatch: expected compiled-substrate identity -> "
            "load-start #1 -> load-complete #1 -> "
            "load-start #2 -> load-complete #2 -> empty prims -> MENU entry "
            "0x800B5244 from ra=0x8001E7C0"
        )

    return Verdict(len(lines), len(starts), len(completions), len(menu_entries))


def _reader(stream: io.TextIOBase, output: queue.Queue[str | None]) -> None:
    try:
        for line in stream:
            output.put(line)
    finally:
        output.put(None)


def _stop_exact_child(process: subprocess.Popen[str]) -> None:
    if process.poll() is not None:
        return
    process.terminate()
    try:
        process.wait(timeout=2.0)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=2.0)


def run_until_boundary(
    arguments: Sequence[str],
    *,
    cwd: Path,
    environment: dict[str, str],
    timeout: float,
    echo: bool = True,
) -> str:
    process = subprocess.Popen(
        list(arguments),
        cwd=cwd,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )
    assert process.stdout is not None
    output: queue.Queue[str | None] = queue.Queue()
    reader = threading.Thread(
        target=_reader, args=(process.stdout, output), daemon=True
    )
    reader.start()
    lines: list[str] = []
    boundary_seen = False
    failure: str | None = None
    deadline = time.monotonic() + timeout
    try:
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                failure = (
                    f"port PID {process.pid} did not reach the nested-MENU boundary "
                    f"in {timeout}s"
                )
                break
            try:
                item = output.get(timeout=min(remaining, 0.1))
            except queue.Empty:
                if process.poll() is not None and not reader.is_alive():
                    failure = (
                        f"port PID {process.pid} exited with {process.returncode} "
                        "before the nested-MENU boundary"
                    )
                    break
                continue
            if item is None:
                failure = (
                    f"port PID {process.pid} exited with {process.returncode} "
                    "before the nested-MENU boundary"
                )
                break
            lines.append(item)
            if echo:
                print(item, end="", flush=True)
            if MENU_ENTRY.search(item):
                boundary_seen = True
                break
    finally:
        _stop_exact_child(process)
        reader.join(timeout=2.0)
        while True:
            try:
                item = output.get_nowait()
            except queue.Empty:
                break
            # Once the declared boundary has been observed, the runner owns process
            # termination.  Do not let output from that cleanup become product
            # evidence about behavior before the boundary.
            if item is not None and not boundary_seen:
                lines.append(item)
                if echo:
                    print(item, end="", flush=True)

    text = "".join(lines)
    if failure is not None or not boundary_seen:
        raise Refused(failure or "nested-MENU boundary was not observed", output=text)
    return text


def run(port: Path, executable: Path, timeout: float) -> str:
    if not port.is_file() or not executable.is_file():
        raise Refused(f"port or retail executable absent: {port}, {executable}")
    if timeout <= 0:
        raise Refused(f"timeout must be positive, got {timeout}")
    environment = dict(os.environ)
    environment.pop("PSXPORT_WATCHDOG", None)
    environment.update(
        PSXPORT_ASSET_DIR=str(ROOT / "external/psxport"),
        PSXPORT_VK_HEADLESS="1",
        PSXPORT_NOPACE="1",
        PSXPORT_NOAUDIO="1",
    )
    DEFAULT_LOG.parent.mkdir(parents=True, exist_ok=True)
    try:
        text = run_until_boundary(
            [str(port), str(executable)],
            cwd=ROOT,
            environment=environment,
            timeout=timeout,
        )
    except Refused as error:
        DEFAULT_LOG.write_text(error.output, encoding="utf-8")
        raise
    DEFAULT_LOG.write_text(text, encoding="utf-8")
    return text


def _fixture(substrate_identity: str) -> str:
    return f"""[watchdog] armed: 3s frame-progress timeout (45s grace until the main presenter is ready)
10 field(s) AGREE, 0 DISAGREE, 0 unresolved
[recomp] generated substrate identity: {substrate_identity}
{LOAD_START}
{LOAD_COMPLETE}
{LOAD_START}
{LOAD_COMPLETE}
{EMPTY_PRIMS}
[crashbash-boundary] MENU entry addr=800B5244 ra=8001E7C0
"""


def selftest() -> bool:
    substrate_identity = "recomp-2026-08-30.2-" + "a" * 64
    fixture = _fixture(substrate_identity)
    passed = 0
    cases = 15
    try:
        verdict = judge(fixture, substrate_identity)
        passed += 1
        print(
            f"PASS positive: {verdict.lines} lines, "
            f"{verdict.load_starts}/{verdict.load_completions} loaded modules, "
            f"{verdict.menu_entry_hits} nested-MENU entry"
        )
    except Refused as error:
        print(f"FAIL positive: {error}", file=sys.stderr)

    menu_entry_line = next(
        line for line in fixture.splitlines() if "[crashbash-boundary]" in line
    )
    negatives = (
        (
            fixture.replace(
                f"[recomp] generated substrate identity: {substrate_identity}\n", ""
            ),
            "missing compiled-substrate identity",
        ),
        (
            fixture.replace(substrate_identity, substrate_identity[:-1] + "b"),
            "wrong compiled-substrate identity",
        ),
        (fixture.replace(LOAD_START + "\n", "", 1), "missing first load start"),
        (fixture.replace(LOAD_COMPLETE + "\n", "", 1), "missing load completion"),
        (
            fixture.replace(
                f"{LOAD_COMPLETE}\n{LOAD_START}",
                f"{LOAD_START}\n{LOAD_COMPLETE}",
            ),
            "interleaved load progression",
        ),
        (fixture.replace(menu_entry_line + "\n", ""), "missing MENU entry"),
        (fixture.replace(MENU_ENTRY_ADDRESS, "800B5218"), "wrong MENU entry"),
        (fixture.replace(MENU_ENTRY_CALLER_RA, "8001E7BC"), "wrong MENU caller"),
        (
            fixture.replace(
                f"{EMPTY_PRIMS}\n{menu_entry_line}",
                f"{menu_entry_line}\n{EMPTY_PRIMS}",
            ),
            "MENU entry before empty-prims output",
        ),
        (fixture + "[watchdog] STUCK: forced negative\n", "watchdog failure"),
        (fixture + "FATAL: forced negative\n", "fatal runtime failure"),
        (fixture + "[recomp-MISS forced-negative]\n", "recompilation miss"),
        (fixture + "VSync: timeout\n", "guest VSync timeout"),
    )
    for changed, label in negatives:
        try:
            judge(changed, substrate_identity)
        except Refused:
            passed += 1
            print(f"PASS negative: {label} is rejected")
        else:
            print(f"FAIL negative: {label} passed", file=sys.stderr)

    scratch = ROOT / "scratch"
    scratch.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=scratch) as directory:
        helper = Path(directory) / "boundary.py"
        helper.write_text(
            "import signal\n"
            "import sys\n"
            "import time\n"
            "def interrupted(_signum, _frame):\n"
            "    print('[watchdog] INTERRUPT: runner-owned cleanup', flush=True)\n"
            "    sys.exit(130)\n"
            "signal.signal(signal.SIGTERM, interrupted)\n"
            f"print('[recomp] generated substrate identity: {substrate_identity}', flush=True)\n"
            f"print({LOAD_START!r}, flush=True)\n"
            f"print({LOAD_COMPLETE!r}, flush=True)\n"
            f"print({LOAD_START!r}, flush=True)\n"
            f"print({LOAD_COMPLETE!r}, flush=True)\n"
            f"print({EMPTY_PRIMS!r}, flush=True)\n"
            f"print({menu_entry_line!r}, flush=True)\n"
            "time.sleep(5)\n",
            encoding="utf-8",
        )
        try:
            output = run_until_boundary(
                [sys.executable, str(helper)],
                cwd=ROOT,
                environment=dict(os.environ),
                timeout=1.0,
                echo=False,
            )
            judge(output, substrate_identity)
            passed += 1
            print(
                "PASS runner: positive boundary stops the exact child process "
                "without judging cleanup output"
            )
        except Refused as error:
            print(f"FAIL runner: {error}", file=sys.stderr)

    print(f"SELFTEST {passed}/{cases}")
    return passed == cases


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group()
    source.add_argument(
        "--trace", type=Path, help="judge an existing product trace without launching"
    )
    source.add_argument(
        "--selftest", action="store_true", help="run hermetic controlled fixtures"
    )
    source.add_argument(
        "--run",
        action="store_true",
        help="explicitly run the serialized product integration boundary",
    )
    parser.add_argument("--port", type=Path, default=DEFAULT_PORT)
    parser.add_argument("--executable", type=Path, default=DEFAULT_EXECUTABLE)
    parser.add_argument("--timeout", type=float, default=30.0)
    args = parser.parse_args()
    try:
        if args.selftest:
            return 0 if selftest() else 1
        if args.trace is not None:
            text = args.trace.read_text(encoding="utf-8", errors="replace")
        elif args.run:
            text = run(args.port, args.executable, args.timeout)
        else:
            raise Refused("select --selftest, --trace PATH, or explicit product --run")
        verdict = judge(text, expected_substrate_identity())
        print(
            f"PASS: {verdict.lines} runtime lines; "
            f"{verdict.load_starts}/{verdict.load_completions} module loads completed, "
            "then empty prims and the game-owned MENU 0x800B5244 observer from "
            "ra=0x8001E7C0; no watchdog stall, fatal error, "
            "recompilation miss, or guest VSync timeout observed"
        )
        return 0
    except (OSError, Refused) as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
