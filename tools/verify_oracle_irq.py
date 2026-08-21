#!/usr/bin/env python3
"""Compare Crash Bash's custom interrupt-exit ordering with a true Beetle oracle trace."""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

ORACLE_EVENT = re.compile(
    r"ORACLE_IRQ pc=(?P<pc>[0-9a-fA-F]{8}) v0=(?P<v0>[0-9a-fA-F]{8}).*"
    r"sp=(?P<sp>[0-9a-fA-F]{8}) ra=(?P<ra>[0-9a-fA-F]{8})"
)
PORT_EVENT = re.compile(r"\[fntrace\] 0x(?P<pc>[0-9A-F]{8})")
PENDING = "[irq] pending I_STAT&I_MASK=0x004"
SHARED_SEQUENCE = (0x80031AE8, 0x80031B58)
PORT_SEQUENCE = SHARED_SEQUENCE + (0x8003F5F0, 0x8003E14C)


class Refused(RuntimeError):
    """The logs do not prove the compared interrupt contract."""


@dataclass(frozen=True)
class Event:
    pc: int
    v0: int
    sp: int
    ra: int


def oracle_pair(text: str) -> tuple[Event, Event]:
    events = [
        Event(**{key: int(value, 16) for key, value in match.groupdict().items()})
        for match in ORACLE_EVENT.finditer(text)
        if int(match.group("pc"), 16) in SHARED_SEQUENCE
    ]
    for first, second in zip(events, events[1:]):
        if (
            (first.pc, second.pc) == SHARED_SEQUENCE
            and first.v0 == second.v0 == 1
            and first.sp == second.sp
            and second.ra == 0x80031AF8
        ):
            return first, second
    raise Refused(
        "oracle lacks adjacent 0x80031AE8 -> 0x80031B58 entries with v0=1, "
        "one saved stack, and dispatcher ra=0x80031AF8"
    )


def port_sequence(text: str) -> tuple[int, ...]:
    marker = text.find(PENDING)
    if marker < 0:
        raise Refused("port trace lacks pending CD interrupt bit 2")
    events = [int(match.group("pc"), 16) for match in PORT_EVENT.finditer(text, marker)]
    found: list[int] = []
    cursor = 0
    for expected in PORT_SEQUENCE:
        while cursor < len(events) and events[cursor] != expected:
            cursor += 1
        if cursor == len(events):
            rendered = " -> ".join(f"0x{pc:08X}" for pc in PORT_SEQUENCE)
            raise Refused(f"port lacks ordered interrupt service {rendered}")
        found.append(events[cursor])
        cursor += 1
    return tuple(found)


def compare(oracle: str, port: str) -> tuple[Event, Event]:
    pair = oracle_pair(oracle)
    observed = port_sequence(port)
    if observed[:2] != tuple(event.pc for event in pair):
        raise Refused("port and oracle custom exception-exit prefixes disagree")
    return pair


def selftest() -> bool:
    oracle = "\n".join(
        (
            "ORACLE_IRQ pc=80031ae8 v0=00000000 sp=801ffcb8 ra=800302d8",
            "ORACLE_IRQ pc=80031ae8 v0=00000001 sp=80068b14 ra=80031ae8",
            "ORACLE_IRQ pc=80031b58 v0=00000001 sp=80068b14 ra=80031af8",
        )
    )
    port = "\n".join(
        (
            PENDING,
            "[fntrace] 0x80031AE8 call #2",
            "[fntrace] 0x80031B58 REACHED",
            "[fntrace] 0x8003F5F0 REACHED",
            "[fntrace] 0x8003E14C REACHED",
        )
    )
    passed = 0
    try:
        compare(oracle, port)
        passed += 1
        print("PASS positive: oracle and port share the custom interrupt-exit prefix")
    except Refused as error:
        print(f"FAIL positive: {error}", file=sys.stderr)

    negatives = (
        (oracle.replace("pc=80031b58", "pc=80031b54"), port, "wrong oracle dispatcher"),
        (oracle.replace("sp=80068b14 ra=80031af8", "sp=80068b10 ra=80031af8"), port, "changed oracle stack"),
        (oracle, port.replace("0x8003F5F0", "0x8003F5EC"), "missing port CD callback"),
    )
    for changed_oracle, changed_port, label in negatives:
        try:
            compare(changed_oracle, changed_port)
        except Refused:
            passed += 1
            print(f"PASS negative: {label} is rejected")
        else:
            print(f"FAIL negative: {label} passed", file=sys.stderr)
    print(f"SELFTEST {passed}/4")
    return passed == 4


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--oracle-log", type=Path)
    parser.add_argument("--port-log", type=Path)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    if args.selftest:
        return 0 if selftest() else 1
    if args.oracle_log is None or args.port_log is None:
        parser.error("--oracle-log and --port-log are required without --selftest")
    try:
        pair = compare(
            args.oracle_log.read_text(encoding="utf-8", errors="replace"),
            args.port_log.read_text(encoding="utf-8", errors="replace"),
        )
    except (OSError, Refused) as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 2
    print(
        "PASS: true oracle and port agree on 0x80031AE8 -> 0x80031B58; "
        f"oracle v0=1 and saved sp=0x{pair[0].sp:08X}; port continues through CD IRQ callback"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
