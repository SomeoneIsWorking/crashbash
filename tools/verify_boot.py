#!/usr/bin/env python3
"""Verify Crash Bash executes its measured loaded modules to the next hardware boundary."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_PORT = ROOT / "scratch/bin/crashbash_port"
EXE = ROOT / "scratch/bin/crashbash/SCUS_945.70"
LOG = ROOT / "scratch/logs/verify-boot.log"
MENU_EXECUTION = "_Z20ov_menu_gen_800B5218P4Core"
POLL_STATE_REACHED = "[fntrace] 0x8002DE2C REACHED"

REQUIRED = (
    "10 field(s) AGREE, 0 DISAGREE, 0 unresolved",
    "[fntrace] 0x8002718C REACHED",
    "[fntrace] 0x8003B1BC REACHED",
    (
        "[irq] pending I_STAT&I_MASK=0x004; no SysEnq element claimed it (1 in chain), "
        "custom exception exit installed"
    ),
    "empty prims",
    MENU_EXECUTION,
    POLL_STATE_REACHED,
    "[watchdog] STUCK: no frame presented within the timeout",
)
FORBIDDEN = (
    "Segmentation fault",
    "CD timeout:",
    "Cant find CRASHBSH.DAT",
    "[recomp-MISS",
)
IRQ_SEQUENCE = ("80031AE8", "80031B58", "8003F5F0", "8003E14C")


class Refused(RuntimeError):
    """The runtime evidence could not be produced."""


@dataclass(frozen=True)
class Verdict:
    lines: int
    required: int
    forbidden: int


def judge(text: str) -> Verdict:
    lines = len(text.splitlines())
    missing = [pattern for pattern in REQUIRED if pattern not in text]
    present = [pattern for pattern in FORBIDDEN if pattern in text]
    load_starts = text.count("load file start")
    load_completions = text.count("done loading")
    marker = text.find(REQUIRED[3])
    sequence_index = marker
    missing_sequence: list[str] = []
    if marker >= 0:
        for address in IRQ_SEQUENCE:
            sequence_index = text.find(f"[fntrace] 0x{address}", sequence_index + 1)
            if sequence_index < 0:
                missing_sequence.append(address)
                break
    if (
        missing
        or present
        or missing_sequence
        or load_starts < 2
        or load_completions < 2
    ):
        details = []
        if missing:
            details.append("missing " + ", ".join(repr(item) for item in missing))
        if present:
            details.append("forbidden " + ", ".join(repr(item) for item in present))
        if load_starts < 2 or load_completions < 2:
            details.append(
                f"loaded-module progression {load_starts} start(s), "
                f"{load_completions} completion(s); expected at least 2/2"
            )
        if missing_sequence:
            details.append(
                "missing ordered IRQ service after pending bit 2: "
                + " -> ".join(f"0x{address}" for address in IRQ_SEQUENCE)
            )
        raise Refused(
            f"runtime boundary failed over {lines} log line(s): {'; '.join(details)}"
        )
    return Verdict(lines, len(REQUIRED), len(FORBIDDEN))


def run(port: Path, timeout: float) -> str:
    if not port.is_file() or not EXE.is_file():
        raise Refused(f"port or retail executable absent: {port}, {EXE}")
    LOG.parent.mkdir(parents=True, exist_ok=True)
    environment = dict(os.environ)
    environment.update(
        PSXPORT_ASSET_DIR=str(ROOT / "external/psxport"),
        PSXPORT_NOPACE="1",
        PSXPORT_NOAUDIO="1",
        PSXPORT_FNTRACE=(
            "8002718C,8003B1BC,80031AE8,80031B58,8003F5F0,8003E14C,8002DE2C"
        ),
        PSXPORT_FNTRACE_REGS="2",
        PSXPORT_WATCHDOG="2",
    )
    process = subprocess.Popen(
        [str(port), str(EXE)],
        cwd=ROOT,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    try:
        output, _ = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired as error:
        process.kill()  # exact child PID owned by this Popen; never a shared-name kill
        output, _ = process.communicate()
        LOG.write_text(output, encoding="utf-8")
        raise Refused(
            f"port PID {process.pid} did not reach a terminal boundary in {timeout}s"
        ) from error
    LOG.write_text(output, encoding="utf-8")
    if process.returncode == 0:
        raise Refused(
            "port returned success instead of stopping at the declared incomplete boundary"
        )
    return output


def selftest(port: Path, timeout: float) -> bool:
    output = run(port, timeout)
    verdict = judge(output)
    print(
        f"PASS positive: {verdict.lines} runtime line(s), {verdict.required}/{len(REQUIRED)} "
        f"required boundary facts, ordered {len(IRQ_SEQUENCE)}-entry IRQ service, "
        f"{verdict.forbidden}/{len(FORBIDDEN)} forbidden patterns absent; "
        "expected next boundary resident CD poll 0x8002DE2C"
    )
    passed = 1
    changed = output.replace(REQUIRED[1], "game-main trace removed", 1)
    try:
        judge(changed)
    except Refused:
        passed += 1
        print("PASS negative: removing the game-main trace fails the boundary")
    else:
        print("FAIL negative: missing game-main trace passed", file=sys.stderr)
    try:
        judge(output + "\n[recomp-MISS forced-negative]\n")
    except Refused:
        passed += 1
        print("PASS negative: a recomp miss fails the boundary")
    else:
        print("FAIL negative: recomp miss passed", file=sys.stderr)
    changed = output.replace("0x80031B58", "0x80031B54")
    try:
        judge(changed)
    except Refused:
        passed += 1
        print("PASS negative: breaking the master-dispatcher order fails the boundary")
    else:
        print("FAIL negative: broken IRQ service order passed", file=sys.stderr)
    changed = output.replace("load file start", "file-load progression removed", 1)
    try:
        judge(changed)
    except Refused:
        passed += 1
        print("PASS negative: missing file-load progression fails the boundary")
    else:
        print("FAIL negative: missing file-load progression passed", file=sys.stderr)
    changed = output.replace(MENU_EXECUTION, "MENU execution removed", 1)
    try:
        judge(changed)
    except Refused:
        passed += 1
        print("PASS negative: removing MENU execution fails the boundary")
    else:
        print("FAIL negative: missing MENU execution passed", file=sys.stderr)
    changed = output.replace(POLL_STATE_REACHED, "resident poll trace removed", 1)
    try:
        judge(changed)
    except Refused:
        passed += 1
        print("PASS negative: removing the resident poll trace fails the boundary")
    else:
        print("FAIL negative: missing resident poll trace passed", file=sys.stderr)
    print(f"SELFTEST {passed}/7")
    return passed == 7


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=Path, default=DEFAULT_PORT)
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    try:
        if args.selftest:
            return 0 if selftest(args.port, args.timeout) else 1
        verdict = judge(run(args.port, args.timeout))
        print(
            f"PASS: {verdict.lines} runtime line(s); crt0, guest main, custom exception exit, "
            "master dispatcher, CD IRQ callback, both loaded modules, and MENU callback observed "
            "before the measured resident CD poll boundary 0x8002DE2C"
        )
        return 0
    except Refused as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
