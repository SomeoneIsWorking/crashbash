#!/usr/bin/env python3
"""Verify Crash Bash reaches its measured first live hardware boundary without a recomp miss."""

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

REQUIRED = (
    "10 field(s) AGREE, 0 DISAGREE, 0 unresolved",
    "[fntrace] 0x8002718C REACHED",
    "[fntrace] 0x8003B1BC REACHED",
    "CD timeout:",
    "[watchdog] STUCK: no frame presented within the timeout",
)
FORBIDDEN = ("[recomp-MISS", "Segmentation fault")


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
    if missing or present:
        details = []
        if missing:
            details.append("missing " + ", ".join(repr(item) for item in missing))
        if present:
            details.append("forbidden " + ", ".join(repr(item) for item in present))
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
        PSXPORT_FNTRACE="8002718C,8003B1BC",
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
        f"required boundary facts, {verdict.forbidden}/{len(FORBIDDEN)} forbidden patterns absent"
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
    print(f"SELFTEST {passed}/3")
    return passed == 3


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
            f"PASS: {verdict.lines} runtime line(s); crt0, guest main, IRQ callback, and CD/VSync "
            "boundary observed with no recomp miss"
        )
        return 0
    except Refused as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
