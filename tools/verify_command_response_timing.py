#!/usr/bin/env python3
"""Diagnose the zero-latency CDC command-response boundary from a register trace."""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

COMMAND = "[cdc] cmd 0x13 params=0"
ANY_COMMAND = "[cdc] cmd "
HANDLER_RESPONSE = re.compile(r"\[cdcr\] r\[1801\]=(?P<byte>[0-9A-F]{2}).*pc=8003E14C")
HANDLER_ACK = re.compile(r"\[cdcw\] w\[1803\]=07.*pc=8003E14C")
CALLER_EMPTY_POLL = re.compile(r"\[cdcr\] r\[1803\]=E0.*pc=8002DE2C")


class Refused(RuntimeError):
    """The trace does not prove the command-response ordering defect."""


@dataclass(frozen=True)
class Boundary:
    response: tuple[int, ...]
    empty_polls: int


def diagnose(text: str) -> Boundary:
    lines = text.splitlines()
    command_indices = [index for index, line in enumerate(lines) if COMMAND in line]
    for command_index in reversed(command_indices):
        response: list[int] = []
        ack_index: int | None = None
        empty_polls = 0
        for index, line in enumerate(lines[command_index + 1 :], command_index + 1):
            if ANY_COMMAND in line:
                break
            match = HANDLER_RESPONSE.search(line)
            if match is not None and ack_index is None:
                response.append(int(match.group("byte"), 16))
            if HANDLER_ACK.search(line) is not None:
                ack_index = index
            if CALLER_EMPTY_POLL.search(line) is not None and ack_index is not None:
                empty_polls += 1
        if len(response) >= 3 and response[-2:] == [1, 1] and empty_polls >= 2:
            return Boundary(tuple(response), empty_polls)
    raise Refused(
        "trace lacks GetTN -> handler response/ack -> repeated empty caller-poll ordering"
    )


def selftest() -> bool:
    observed = f"""{COMMAND}
[cdcr] r[1803]=E3 bank=1 irq pc=8003E14C ra=8003F62C
[cdcr] r[1801]=02 bank=1 resp pc=8003E14C ra=8003F62C
[cdcr] r[1801]=01 bank=1 resp pc=8003E14C ra=8003F62C
[cdcr] r[1801]=01 bank=1 resp pc=8003E14C ra=8003F62C
[cdcw] w[1803]=07 bank=1 pc=8003E14C ra=8003F62C
[cdcr] r[1803]=E0 bank=1 irq pc=8002DE2C ra=8002D560
[cdcr] r[1803]=E0 bank=1 irq pc=8002DE2C ra=8002D560"""
    passed = 0
    try:
        boundary = diagnose(observed)
        passed += 1
        print(
            "PASS positive: handler drained GetTN response "
            f"{boundary.response} before {boundary.empty_polls} empty caller polls"
        )
    except Refused as error:
        print(f"FAIL positive: {error}", file=sys.stderr)

    negatives = (
        (observed.replace(COMMAND, "[cdc] cmd 0x01 params=0"), "missing GetTN"),
        (observed.replace("r[1801]=01", "r[1801]=00"), "wrong response bytes"),
        (observed.replace("w[1803]=07", "w[1803]=00"), "missing handler ack"),
        (
            observed.replace("r[1803]=E0", "r[1803]=E3"),
            "caller observes the response",
        ),
    )
    for changed, label in negatives:
        try:
            diagnose(changed)
        except Refused:
            passed += 1
            print(f"PASS negative: {label} is rejected")
        else:
            print(f"FAIL negative: {label} passed", file=sys.stderr)
    print(f"SELFTEST {passed}/5")
    return passed == 5


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", nargs="?", type=Path)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    if args.selftest:
        return 0 if selftest() else 1
    if args.trace is None:
        parser.error("trace is required without --selftest")
    try:
        boundary = diagnose(args.trace.read_text(encoding="utf-8", errors="replace"))
    except (OSError, Refused) as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 2
    print(
        "PASS: GetTN response was drained and acknowledged by 0x8003E14C before "
        f"0x8002DE2C observed {boundary.empty_polls} empty interrupt-flag polls"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
