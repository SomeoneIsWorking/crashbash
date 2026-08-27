#!/usr/bin/env python3
"""Verify Crash Bash's native frame ownership against retail bytes and shipping wiring."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXECUTABLE = ROOT / "scratch/bin/crashbash/SCUS_945.70"
IDENTITY = ROOT / "titles/crashbash/executable.json"
BOOT_IDENTITY = ROOT / "titles/crashbash/boot_module.json"
BOOT_PAYLOAD = ROOT / "scratch/bin/crashbash/overlays/BOOT.BIN"
GUEST_HEADER = ROOT / "game/core/crashbash_guest.h"
CONFIG = ROOT / "game/core/game_config.cpp"
RUNTIME = ROOT / "game/core/crashbash_runtime.cpp"
CD_FILE_READ = ROOT / "game/core/cd_file_read.cpp"
CD_LICENSE_STARTUP = ROOT / "game/core/cd_license_startup.cpp"
CD_STARTUP = ROOT / "game/core/cd_startup.cpp"
BOOT = ROOT / "game/core/crashbash_boot.cpp"
FRAME_DRIVER = ROOT / "game/core/crashbash_frame_driver.cpp"
DISPLAY = ROOT / "game/core/display_frame.cpp"
GPU_TIMEOUT = ROOT / "game/core/gpu_timeout.cpp"
MEMORY_CARD_STARTUP = ROOT / "game/core/memory_card_startup.cpp"

# Each PC is the JAL instruction whose return address is visible in the emitted retail body.
DISPLAY_VSYNC_CALL_SITES = (0x80010530, 0x80010570, 0x8001067C, 0x80027378, 0x80027388)
GPU_TIMEOUT_VSYNC_CALL_SITES = (0x80031274, 0x800312A8, 0x8003168C)
MEMORY_CARD_VSYNC_CALL_SITES = (0x800486F8,)
CD_SYNC_VSYNC_CALL_SITES = (0x8003E6E4, 0x8003E730, 0x8003EDB4, 0x8003EE0C)
CD_READY_VSYNC_CALL_SITES = (0x80034A50, 0x80034A94)
CD_LICENSE_VSYNC_CALL_SITES = (
    0x8002D9DC,
    0x8002DA00,
    0x8002DBA0,
    0x8002DBC4,
    0x8002DC70,
    0x8002DDC4,
)
OWNED_VSYNC_CALL_SITES = (
    DISPLAY_VSYNC_CALL_SITES
    + GPU_TIMEOUT_VSYNC_CALL_SITES
    + MEMORY_CARD_VSYNC_CALL_SITES
    + CD_SYNC_VSYNC_CALL_SITES
    + CD_READY_VSYNC_CALL_SITES
    + CD_LICENSE_VSYNC_CALL_SITES
)
REQUIRED_CONSTANTS = {
    "kGameMain": 0x8002718C,
    "kApplicationMain": 0x80010158,
    "kCdFileRead": 0x80027790,
    "kCdLicenseStartup": 0x8002D4F4,
    "kDisplayFrame": 0x800272AC,
    "kGpuTimeoutArm": 0x8003126C,
    "kGpuTimeoutCheck": 0x800312A0,
    "kGpuTransfer": 0x8003165C,
    "kCdDriveReady": 0x800349AC,
    "kCdInitHandshake": 0x80034B8C,
    "kCdSearchFile": 0x80034C6C,
    "kCdSync": 0x8003E6B0,
    "kCdCommand": 0x8003EBF8,
    "kMemoryCardStartup": 0x800486DC,
    "kInitialProcessState": 0x8004E0B8,
    "kCurrentProcessState": 0x8005B648,
    "kInitialStateEnter": 0x80010410,
    "kInitialStateUpdate": 0x80010394,
    "kInitialStatePresent": 0x80010278,
    "kDisplayFieldsPerFrame": 0x8004E0E0,
    "kVblankCounter": 0x8006D8DC,
    "kVblankRoot": 0x8003ADD4,
    "kCdReadActive": 0x800637B4,
    "kCdBaseLba": 0x800637B8,
    "kCdLicenseState": 0x80067894,
    "kDiscExecutableLba": 0x00000017,
    "kDiscExecutableSize": 0x00069800,
    "kDiscSystemCnfLba": 0x000000EA,
    "kDiscSystemCnfSize": 0x00000045,
    "kDiscDataLba": 0x000000EC,
    "kDiscDataSize": 0x045D4000,
    "kDiscTrackSectors": 0x000127FE,
}


class Refused(RuntimeError):
    """Shipping declarations do not agree with measured ownership evidence."""


def _constant(text: str, name: str) -> int:
    match = re.search(rf"\b{name}\b\s*=\s*0x([0-9A-Fa-f]+)u?", text)
    if not match:
        raise Refused(f"missing typed guest constant {name}")
    return int(match.group(1), 16)


def _vsync_range(text: str) -> tuple[int, int]:
    match = re.search(
        r"\bkVSync\s*\{\s*0x([0-9A-Fa-f]+)u?\s*,\s*0x([0-9A-Fa-f]+)u?\s*\}",
        text,
    )
    if not match:
        raise Refused("missing typed kVSync range")
    return int(match.group(1), 16), int(match.group(2), 16)


def _word(image: bytes, load: int, address: int) -> int:
    offset = 0x800 + address - load
    if offset < 0 or offset + 4 > len(image):
        raise Refused(f"retail address 0x{address:08X} lies outside the executable")
    return struct.unpack_from("<I", image, offset)[0]


def _jal_target(pc: int, instruction: int) -> int:
    if instruction >> 26 != 3:
        raise Refused(f"retail 0x{pc:08X} is not JAL: 0x{instruction:08X}")
    return ((pc + 4) & 0xF0000000) | ((instruction & 0x03FFFFFF) << 2)


def _jal_sites(
    blob: bytes, data_offset: int, load: int, size: int, target: int
) -> tuple[int, ...]:
    opcode = 0x0C000000 | ((target >> 2) & 0x03FFFFFF)
    return tuple(
        load + offset
        for offset in range(0, size, 4)
        if struct.unpack_from("<I", blob, data_offset + offset)[0] == opcode
    )


def _verify_full_inventory(
    image: bytes,
    identity: dict[str, object],
    boot: bytes,
    boot_identity: dict[str, object],
    vsync: int,
) -> None:
    load = int(str(identity["header"]["text_address"]), 0)  # type: ignore[index]
    size = int(str(identity["header"]["text_size"]), 0)  # type: ignore[index]
    resident_sites = _jal_sites(image, 0x800, load, size, vsync)
    boot_load = int(str(boot_identity["load_address"]), 0)
    boot_sites = _jal_sites(boot, 0, boot_load, len(boot), vsync)
    if len(resident_sites) != 47 or len(boot_sites) != 4:
        raise Refused(
            f"retail VSync inventory is resident={len(resident_sites)}, BOOT={len(boot_sites)}; "
            "measured owner map requires 47+4"
        )


def verify(
    image: bytes,
    identity: dict[str, object],
    guest_text: str,
    config_text: str,
    runtime_text: str,
    cd_file_read_text: str,
    cd_license_startup_text: str,
    cd_startup_text: str,
    boot_text: str,
    frame_text: str,
    display_text: str,
    gpu_timeout_text: str,
    memory_card_text: str,
) -> None:
    load = int(str(identity["header"]["text_address"]), 0)  # type: ignore[index]
    vsync_begin, vsync_end = _vsync_range(guest_text)
    if vsync_end != vsync_begin + 4:
        raise Refused(
            f"kVSync must cover exactly one entry instruction, got "
            f"[0x{vsync_begin:08X},0x{vsync_end:08X})"
        )
    for name, expected in REQUIRED_CONSTANTS.items():
        actual = _constant(guest_text, name)
        if actual != expected:
            raise Refused(f"{name}=0x{actual:08X}, measured 0x{expected:08X}")

    state = _constant(guest_text, "kInitialProcessState")
    expected_table = tuple(
        _constant(guest_text, name)
        for name in (
            "kInitialStateEnter",
            "kInitialStateUpdate",
            "kInitialStatePresent",
        )
    )
    actual_table = tuple(_word(image, load, state + 4 * index) for index in range(3))
    if actual_table != expected_table:
        raise Refused(
            "retail initial process-state table is "
            + ", ".join(f"0x{value:08X}" for value in actual_table)
            + "; typed declarations say "
            + ", ".join(f"0x{value:08X}" for value in expected_table)
        )

    for pc in OWNED_VSYNC_CALL_SITES:
        target = _jal_target(pc, _word(image, load, pc))
        if target != vsync_begin:
            raise Refused(
                f"retail call site 0x{pc:08X} targets 0x{target:08X}, "
                f"typed VSync is 0x{vsync_begin:08X}"
            )

    required_fragments = {
        "GameConfig VSync window": "crashbash::guest::kVSync.end",
        "GameConfig fatal trap": ".vsyncTrap = crashbash::guest::kVSync.begin",
        "runtime FrameDriver factory": "std::make_unique<CrashBashFrameDriver>(game)",
        "runtime finite boot prefix": "runBootPrefix(core)",
        "synchronous file-read registration": "registerCdFileReadOverride()",
        "synchronous file-read retained super": "gen_func_80027790",
        "synchronous file-read real-sector transfer": "disc_read_sector(&core->game->disc",
        "synchronous file-read completion state": "core->mem_w32(guest::kCdReadActive, 0u)",
        "CD license startup registration": "registerCdLicenseStartupOverride()",
        "CD license startup retained super": "gen_func_8002D4F4",
        "CD license startup measured disc proof": "hasMeasuredDiscLayout(disc)",
        "CD license startup failure-path exclusion": "copy-protection failure screen",
        "CD license startup passed state": "core->mem_w32(guest::kCdLicenseState, kPassedState)",
        "CD startup registration": "registerCdStartupOverride()",
        "CD startup retained super": "gen_func_80034B8C",
        "CD startup synchronous owner": "cdInitHandshakeOwned",
        "CD readiness retained super": "gen_func_800349AC",
        "CD readiness native TOC proof": "disc_open(&disc) && disc.track_count != 0u",
        "native CD command binding": ".cdCommand = crashbash::guest::kCdCommand",
        "native CD sync binding": ".cdSync = crashbash::guest::kCdSync",
        "native ISO lookup binding": ".cdSearchFile = crashbash::guest::kCdSearchFile",
        "boot runner split": "beginProcessRunnerActivation(core)",
        "frame-owned display delivery": "frameDriver(*core).deliverDisplayFields(*core, fields)",
        "GPU timeout registration": "registerGpuTimeoutOverrides()",
        "GPU arm retained super": "gen_func_8003126C",
        "GPU check retained super": "gen_func_800312A0",
        "GPU transfer retained super": "gen_func_8003165C",
        "memory-card startup registration": "registerMemoryCardStartupOverride()",
        "memory-card startup retained super": "gen_func_800486DC",
        "memory-card startup owner": "memoryCardStartupOwned",
        "synchronous GPU completion state": "armSynchronousGpuTimeout(*core)",
        "single presented commit": "game_.presentation.commit(&core",
        "unpresented transition fence": "game_.presentation.commitUnpresented(&core)",
    }
    sources = (
        config_text,
        runtime_text,
        cd_file_read_text,
        cd_license_startup_text,
        cd_startup_text,
        boot_text,
        frame_text,
        display_text,
        gpu_timeout_text,
        memory_card_text,
    )
    for label, fragment in required_fragments.items():
        if not any(fragment in source for source in sources):
            raise Refused(f"missing shipping ownership wiring: {label}")

    joined_sources = "\n".join(sources)
    if "measuredGuestCall(core, guest::kVSync" in joined_sources:
        raise Refused("title-owned code still dispatches guest VSync")
    if re.search(r"\b(?:func|gen_func)_800320EC\s*\(", joined_sources):
        raise Refused("title-owned code directly calls the guest VSync body")
    if "hSyncCounter(" in gpu_timeout_text or "frameTick(" in gpu_timeout_text:
        raise Refused("synchronous GPU owner samples or advances display timing")
    if re.search(r"\b(?:for|while)\s*\(", gpu_timeout_text):
        raise Refused("synchronous GPU owner contains a polling loop")
    if "rec_dispatch(core, guest::kVSync" in memory_card_text:
        raise Refused("memory-card startup still dispatches guest VSync")


def _retail_inputs() -> tuple[bytes, dict[str, object]]:
    identity = json.loads(IDENTITY.read_text(encoding="utf-8"))
    image = EXECUTABLE.read_bytes()
    digest = hashlib.sha256(image).hexdigest()
    if len(image) != identity["file_size"] or digest != identity["sha256"]:
        raise Refused(
            f"{EXECUTABLE} is not verified SCUS_945.70: {len(image)} bytes sha256 {digest}"
        )
    return image, identity


def verify_shipping() -> None:
    image, identity = _retail_inputs()
    boot_identity = json.loads(BOOT_IDENTITY.read_text(encoding="utf-8"))
    guest_text = GUEST_HEADER.read_text(encoding="utf-8")
    manifest_disc_facts = {
        "kDiscExecutableLba": identity["disc_lba"],
        "kDiscExecutableSize": identity["file_size"],
        "kDiscSystemCnfLba": identity["system_cnf_lba"],
        "kDiscSystemCnfSize": identity["system_cnf_size"],
        "kDiscDataLba": boot_identity["source_file_lba"],
        "kDiscDataSize": boot_identity["source_size"],
        "kDiscTrackSectors": identity["disc_track_sectors"],
    }
    for name, expected in manifest_disc_facts.items():
        actual = _constant(guest_text, name)
        if actual != expected:
            raise Refused(f"{name}=0x{actual:08X}, manifest measures 0x{expected:08X}")
    boot = BOOT_PAYLOAD.read_bytes()
    boot_digest = hashlib.sha256(boot).hexdigest()
    if (
        len(boot) != int(str(boot_identity["payload_size"]), 0)
        or boot_digest != boot_identity["payload_sha256"]
    ):
        raise Refused(f"{BOOT_PAYLOAD} does not match the verified BOOT payload")
    verify(
        image,
        identity,
        guest_text,
        CONFIG.read_text(encoding="utf-8"),
        RUNTIME.read_text(encoding="utf-8"),
        CD_FILE_READ.read_text(encoding="utf-8"),
        CD_LICENSE_STARTUP.read_text(encoding="utf-8"),
        CD_STARTUP.read_text(encoding="utf-8"),
        BOOT.read_text(encoding="utf-8"),
        FRAME_DRIVER.read_text(encoding="utf-8"),
        DISPLAY.read_text(encoding="utf-8"),
        GPU_TIMEOUT.read_text(encoding="utf-8"),
        MEMORY_CARD_STARTUP.read_text(encoding="utf-8"),
    )
    _verify_full_inventory(
        image, identity, boot, boot_identity, _vsync_range(GUEST_HEADER.read_text())[0]
    )


def selftest() -> bool:
    guest_text = GUEST_HEADER.read_text(encoding="utf-8")
    sources = [
        CONFIG.read_text(encoding="utf-8"),
        RUNTIME.read_text(encoding="utf-8"),
        CD_FILE_READ.read_text(encoding="utf-8"),
        CD_LICENSE_STARTUP.read_text(encoding="utf-8"),
        CD_STARTUP.read_text(encoding="utf-8"),
        BOOT.read_text(encoding="utf-8"),
        FRAME_DRIVER.read_text(encoding="utf-8"),
        DISPLAY.read_text(encoding="utf-8"),
        GPU_TIMEOUT.read_text(encoding="utf-8"),
        MEMORY_CARD_STARTUP.read_text(encoding="utf-8"),
    ]
    load = 0x80010000
    image = bytearray(0x800 + 0x69000)
    state = REQUIRED_CONSTANTS["kInitialProcessState"]
    for index, name in enumerate(
        ("kInitialStateEnter", "kInitialStateUpdate", "kInitialStatePresent")
    ):
        struct.pack_into(
            "<I", image, 0x800 + state - load + 4 * index, REQUIRED_CONSTANTS[name]
        )
    vsync = _vsync_range(guest_text)[0]
    jal = 0x0C000000 | ((vsync >> 2) & 0x03FFFFFF)
    for pc in OWNED_VSYNC_CALL_SITES:
        struct.pack_into("<I", image, 0x800 + pc - load, jal)
    identity: dict[str, object] = {"header": {"text_address": hex(load)}}

    cases: list[tuple[str, bytes, str, list[str]]] = [
        ("positive", bytes(image), guest_text, sources),
    ]
    wrong_table = bytearray(image)
    struct.pack_into("<I", wrong_table, 0x800 + state - load + 4, 0x80010390)
    cases.append(("mutated state table", bytes(wrong_table), guest_text, sources))
    wrong_call = bytearray(image)
    struct.pack_into("<I", wrong_call, 0x800 + OWNED_VSYNC_CALL_SITES[-1] - load, 0)
    cases.append(("mutated VSync call", bytes(wrong_call), guest_text, sources))
    cases.append(
        (
            "missing fatal trap",
            bytes(image),
            guest_text,
            [
                sources[0].replace(".vsyncTrap = crashbash::guest::kVSync.begin", ""),
                *sources[1:],
            ],
        )
    )
    cases.append(
        (
            "GPU timing query",
            bytes(image),
            guest_text,
            [
                *sources[:-2],
                sources[-2] + "\nvoid bad() { hSyncCounter(); }\n",
                sources[-1],
            ],
        )
    )
    cases.append(
        (
            "memory-card VSync dispatch",
            bytes(image),
            guest_text,
            [
                *sources[:-1],
                sources[-1]
                + "\nvoid bad(Core *core) { rec_dispatch(core, guest::kVSync); }\n",
            ],
        )
    )
    cases.append(
        (
            "CD-license direct VSync call",
            bytes(image),
            guest_text,
            [
                *sources[:3],
                sources[3] + "\nvoid bad(Core *core) { func_800320EC(core); }\n",
                *sources[4:],
            ],
        )
    )

    passed = 0
    with tempfile.TemporaryDirectory(dir=ROOT / "scratch"):
        for index, (label, candidate, header, candidate_sources) in enumerate(cases):
            try:
                verify(candidate, identity, header, *candidate_sources)
            except Refused:
                if index == 0:
                    print(f"FAIL positive: {label}", file=sys.stderr)
                else:
                    passed += 1
                    print(f"PASS negative: {label} is rejected")
            else:
                if index == 0:
                    passed += 1
                    print("PASS positive: binary ownership and shipping wiring agree")
                else:
                    print(f"FAIL negative: {label} passed", file=sys.stderr)
    print(f"SELFTEST {passed}/{len(cases)}")
    return passed == len(cases)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    try:
        if args.selftest:
            return 0 if selftest() else 1
        verify_shipping()
        print(
            "PASS: retail process table and the migrated display/allocator/GPU-timeout/"
            "memory-card/CD VSync call sites agree with the typed native boot/frame ownership wiring"
        )
        return 0
    except (OSError, KeyError, ValueError, json.JSONDecodeError, Refused) as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
