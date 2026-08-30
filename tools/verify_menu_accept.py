#!/usr/bin/env python3
"""Verify that the active Crash Bash menu accepts Cross and deliberately ignores START."""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

import verify_boot

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_PORT = ROOT / "scratch/bin/crashbash_port"
DEFAULT_EXECUTABLE = ROOT / "scratch/bin/crashbash/SCUS_945.70"
LOG_DIR = ROOT / "scratch/logs"

INPUT = re.compile(
    r"\[crashbash-boundary(?::debug)?\]\s+MENU input "
    r"edge=([0-9A-F]{8}) current=([0-9A-F]{8}) pending=([0-9A-F]{8}) "
    r"state-index=([0-9A-F]{8}) selection=([0-9A-F]{8})",
    re.IGNORECASE,
)
ACCEPT = re.compile(
    r"\[crashbash-boundary\]\s+MENU accept "
    r"edge=([0-9A-F]{8}) current=([0-9A-F]{8}) "
    r"pending=([0-9A-F]{8})->([0-9A-F]{8}) selection=([0-9A-F]{8})",
    re.IGNORECASE,
)


class Refused(RuntimeError):
    """The three traces do not prove the measured retail menu behavior."""


@dataclass(frozen=True)
class Verdict:
    idle_updates: int
    start_updates: int
    start_edges: int
    cross_updates: int


def _records(text: str, pattern: re.Pattern[str]) -> list[tuple[int, ...]]:
    return [tuple(int(value, 16) for value in match.groups()) for match in pattern.finditer(text)]


def _check_trace(name: str, text: str, identity: str) -> tuple[list[tuple[int, ...]], list[tuple[int, ...]]]:
    for pattern in verify_boot.FORBIDDEN:
        if pattern.search(text):
            raise Refused(f"{name}: forbidden runtime failure {pattern.pattern!r}")
    identity_line = f"[recomp] generated substrate identity: {identity}"
    if text.splitlines().count(identity_line) != 1:
        raise Refused(f"{name}: exact compiled-substrate identity denominator is not 1")
    updates = _records(text, INPUT)
    accepts = _records(text, ACCEPT)
    if not updates:
        raise Refused(f"{name}: active MENU update observer produced no records")
    return updates, accepts


def judge(idle: str, start: str, cross: str, identity: str) -> Verdict:
    idle_updates, idle_accepts = _check_trace("idle", idle, identity)
    start_updates, start_accepts = _check_trace("START", start, identity)
    cross_updates, cross_accepts = _check_trace("Cross", cross, identity)

    idle_edges = [record[0] for record in idle_updates if record[0] != 0]
    start_edges = [record[0] for record in start_updates if record[0] != 0]
    cross_edges = [record[0] for record in cross_updates if record[0] != 0]
    expected_checkpoint = (0x800B8E28, 0, 0x28, 0)
    wrong_idle = [record for record in idle_updates if record[1:] != expected_checkpoint]
    wrong_start = [record for record in start_updates if record[1:] != expected_checkpoint]
    if wrong_idle or wrong_start:
        raise Refused(f"idle/START: MENU update escaped the measured checkpoint: {wrong_idle or wrong_start}")
    if idle_edges or idle_accepts:
        raise Refused("idle: input edge or accept path fired")
    if len(idle_updates) != len(start_updates):
        raise Refused(
            f"START: active-menu denominator changed from idle {len(idle_updates)} "
            f"to {len(start_updates)}"
        )
    if not start_edges or any(edge != 8 for edge in start_edges) or start_accepts:
        raise Refused(f"START: expected one or more edge=8 records and zero accepts, got {start_edges}")
    if cross_edges != [0x4000]:
        raise Refused(f"Cross: expected exactly one edge=0x4000, got {cross_edges}")
    cross_edge_index = next(index for index, record in enumerate(cross_updates) if record[0] == 0x4000)
    expected_pending = (0x800B8E28, 0x800B8E50, 0x28, 0)
    wrong_cross_before = [
        record for record in cross_updates[: cross_edge_index + 1] if record[1:] != expected_checkpoint
    ]
    wrong_cross_after = [record for record in cross_updates[cross_edge_index + 1 :] if record[1:] != expected_pending]
    if wrong_cross_before or wrong_cross_after:
        raise Refused(
            "Cross: pending-manager transition disagrees with the measured two-phase checkpoint: "
            f"{wrong_cross_before or wrong_cross_after}"
        )
    expected_accept = (0x4000, 0x800B8E28, 0, 0x800B8E50, 0)
    if cross_accepts != [expected_accept]:
        raise Refused(f"Cross: expected one measured accept/schedule record, got {cross_accepts}")
    if len(cross_updates) >= len(idle_updates):
        raise Refused("Cross: active MENU callback did not leave its idle update sequence")
    return Verdict(len(idle_updates), len(start_updates), len(start_edges), len(cross_updates))


def _identity_line(identity: str) -> str:
    return f"[recomp] generated substrate identity: {identity}\n"


def _input(edge: int, pending: int = 0) -> str:
    return (
        "[crashbash-boundary:debug] MENU input "
        f"edge={edge:08X} current=800B8E28 pending={pending:08X} "
        "state-index=00000028 selection=00000000\n"
    )


def selftest() -> bool:
    identity = "recomp-2026-08-30.3-" + "a" * 64
    idle = _identity_line(identity) + _input(0) * 4
    start = _identity_line(identity) + _input(0) + _input(8) + _input(0) * 2
    cross = (
        _identity_line(identity)
        + _input(0)
        + _input(0x4000)
        + "[crashbash-boundary] MENU accept edge=00004000 current=800B8E28 "
        "pending=00000000->800B8E50 selection=00000000\n"
        + _input(0, 0x800B8E50)
    )
    cases = (
        (idle.replace(_identity_line(identity), ""), start, cross, "missing identity"),
        (idle.replace(_input(0), _input(8), 1), start, cross, "idle edge"),
        (idle, start + cross.splitlines()[-1] + "\n", cross, "START accept"),
        (idle, start.replace(_input(8), _input(0)), cross, "missing START edge"),
        (idle, start, cross.replace("800B8E50", "800B8E54"), "wrong pending table"),
        (idle, start, cross.rsplit("[crashbash-boundary]", 1)[0], "missing Cross accept"),
    )
    passed = 0
    try:
        verdict = judge(idle, start, cross, identity)
        passed += 1
        print(
            f"PASS positive: idle/START={verdict.idle_updates}, START edges={verdict.start_edges}, "
            f"Cross updates={verdict.cross_updates} with one accept"
        )
    except Refused as error:
        print(f"FAIL positive: {error}", file=sys.stderr)
    for changed_idle, changed_start, changed_cross, label in cases:
        try:
            judge(changed_idle, changed_start, changed_cross, identity)
        except Refused:
            passed += 1
            print(f"PASS negative: {label} is rejected")
        else:
            print(f"FAIL negative: {label} passed", file=sys.stderr)
    total = 1 + len(cases)
    print(f"SELFTEST {passed}/{total}")
    return passed == total


def run_variant(port: Path, executable: Path, name: str, buttons: str | None, timeout: float) -> str:
    environment = dict(os.environ)
    for key in ("PSXPORT_FORCE_BUTTONS", "PSXPORT_FORCE_HOLD", "PSXPORT_FORCE_HOLD_AT"):
        environment.pop(key, None)
    environment.update(
        PSXPORT_ASSET_DIR=str(ROOT / "external/psxport"),
        PSXPORT_DEBUG="crashbash-boundary",
        PSXPORT_NATIVE_FRAMES="600",
        PSXPORT_NOAUDIO="1",
        PSXPORT_NOPACE="1",
        PSXPORT_PRODUCERS="0",
        PSXPORT_VK_HEADLESS="1",
    )
    if buttons is not None:
        environment["PSXPORT_FORCE_BUTTONS"] = buttons
    try:
        result = subprocess.run(
            [str(port), str(executable)],
            cwd=ROOT,
            env=environment,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired as error:
        raise Refused(f"{name}: product exceeded {timeout}s") from error
    text = result.stdout + result.stderr
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    (LOG_DIR / f"verify-menu-accept-{name.lower()}.log").write_text(text, encoding="utf-8")
    if result.returncode != 0:
        raise Refused(f"{name}: product exited {result.returncode}")
    return text


def run(port: Path, executable: Path, timeout: float) -> Verdict:
    if not port.is_file() or not executable.is_file():
        raise Refused(f"port or retail executable absent: {port}, {executable}")
    identity = verify_boot.expected_substrate_identity()
    idle = run_variant(port, executable, "idle", None, timeout)
    start = run_variant(port, executable, "start", "FFF7", timeout)
    cross = run_variant(port, executable, "cross", "BFFF", timeout)
    return judge(idle, start, cross, identity)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--selftest", action="store_true")
    source.add_argument("--run", action="store_true")
    parser.add_argument("--port", type=Path, default=DEFAULT_PORT)
    parser.add_argument("--executable", type=Path, default=DEFAULT_EXECUTABLE)
    parser.add_argument("--timeout", type=float, default=30.0)
    args = parser.parse_args()
    try:
        if args.selftest:
            return 0 if selftest() else 1
        verdict = run(args.port, args.executable, args.timeout)
        print(
            f"PASS: idle/START each executed {verdict.idle_updates} active MENU updates; "
            f"{verdict.start_edges} START edge(s) produced no accept; Cross left after "
            f"{verdict.cross_updates} update(s) and scheduled 0x800B8E50 exactly once"
        )
        return 0
    except (OSError, Refused) as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
