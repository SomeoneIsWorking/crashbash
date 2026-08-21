#!/usr/bin/env python3
"""Compare Crash Bash's 189-sector read completion with a true oracle trace."""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

SECTORS = 189
SECTOR_BYTES = 2048
EVENT = re.compile(
    r"^(?P<source>ORACLE|PORT)_READ event=(?P<event>[a-z-]+).*?"
    r"v0=(?P<v0>[0-9a-fA-F]{8}).*?active=(?P<active>[0-9a-fA-F]{8}) "
    r"requested=(?P<requested>[0-9a-fA-F]{8}) "
    r"remaining=(?P<remaining>[0-9a-fA-F]{8}) "
    r"dest=(?P<dest>[0-9a-fA-F]{8}) "
    r"expected=(?P<expected>[0-9a-fA-F]{8}) "
    r"async=(?P<async_flag>[0-9a-fA-F]{8})(?: .*)?$",
    re.MULTILINE,
)


class Refused(RuntimeError):
    """The trace does not prove the retail read-completion contract."""


@dataclass(frozen=True)
class Snapshot:
    source: str
    event: str
    v0: int
    active: int
    requested: int
    remaining: int
    dest: int
    expected: int
    async_flag: int


@dataclass(frozen=True)
class Contract:
    initial: Snapshot
    completed: Snapshot
    settled: Snapshot


def snapshots(text: str, source: str) -> list[Snapshot]:
    result = []
    for match in EVENT.finditer(text):
        values = match.groupdict()
        if values["source"] != source:
            continue
        result.append(
            Snapshot(
                source=values["source"],
                event=values["event"],
                **{
                    name: int(values[name], 16)
                    for name in (
                        "v0",
                        "active",
                        "requested",
                        "remaining",
                        "dest",
                        "expected",
                        "async_flag",
                    )
                },
            )
        )
    if not result:
        raise Refused(f"{source.lower()} trace contains no read-state events")
    return result


def completion_contract(events: list[Snapshot], source: str) -> Contract:
    initial_index = next(
        (
            index
            for index, event in enumerate(events)
            if event.event == "read-return"
            and event.requested == SECTORS
            and event.v0 == SECTORS
            and event.active == SECTORS
            and event.remaining == SECTORS
            and event.async_flag == 0
        ),
        None,
    )
    if initial_index is None:
        observed = [
            f"v0={event.v0:#x}/active={event.active:#x}/remaining={event.remaining:#x}"
            for event in events
            if event.event == "read-return" and event.requested == SECTORS
        ]
        detail = ", ".join(observed) if observed else "no 189-sector read return"
        raise Refused(
            f"{source} does not return 189 while the 189-sector read is active: {detail}"
        )
    initial = events[initial_index]
    expected_dest = initial.dest + SECTORS * SECTOR_BYTES
    expected_sector = initial.expected + SECTORS

    completed_index = next(
        (
            index
            for index in range(initial_index + 1, len(events))
            if events[index].event == "sync-return"
            and events[index].requested == SECTORS
            and events[index].v0 == 1
            and events[index].remaining == 0
            and events[index].async_flag == 1
            and events[index].dest == expected_dest
            and events[index].expected == expected_sector
        ),
        None,
    )
    if completed_index is None:
        transient = any(
            event.event == "async-write"
            and event.requested == SECTORS
            and event.v0 == 1
            and event.remaining == 0
            and event.async_flag == 1
            and event.dest == expected_dest
            and event.expected == expected_sector
            for event in events[initial_index + 1 :]
        )
        if transient:
            raise Refused(
                f"{source} reaches completion-pending state only inside IRQ handling; "
                "no sync call returns it"
            )
        raise Refused(
            f"{source} lacks the completed-but-callback-pending result after 189 sectors"
        )
    completed = events[completed_index]
    settled = next(
        (
            event
            for event in events[completed_index + 1 :]
            if event.event == "sync-return"
            and event.requested == SECTORS
            and event.v0 == 0
            and event.remaining == 0
            and event.async_flag == 0
            and event.dest == expected_dest
            and event.expected == expected_sector
        ),
        None,
    )
    if settled is None:
        raise Refused(f"{source} never settles the completed read from result 1 to 0")
    repeated = [
        event
        for event in events[initial_index + 1 :]
        if event.event == "read-return"
        and event.requested == SECTORS
        and event.dest == initial.dest
        and event.expected == initial.expected
    ]
    if repeated:
        raise Refused(f"{source} restarts the same 189-sector read {len(repeated)} time(s)")
    return Contract(initial, completed, settled)


def compare(oracle_text: str, port_text: str) -> tuple[Contract, Contract]:
    oracle = completion_contract(snapshots(oracle_text, "ORACLE"), "oracle")
    port = completion_contract(snapshots(port_text, "PORT"), "port")
    oracle_key = (
        oracle.initial.dest,
        oracle.initial.expected,
        oracle.completed.dest,
        oracle.completed.expected,
    )
    port_key = (
        port.initial.dest,
        port.initial.expected,
        port.completed.dest,
        port.completed.expected,
    )
    if oracle_key != port_key:
        raise Refused("port and oracle disagree on the read destination/sector interval")
    return oracle, port


def sample(source: str, *, instant: bool = False, repeat: bool = False) -> str:
    prefix = f"{source}_READ"
    if instant:
        return (
            f"{prefix} event=read-return v0=ffffffff active=00000000 "
            "requested=000000bd remaining=00000000 dest=800d7490 "
            "expected=00008c94 async=00000000\n"
        )
    lines = [
        f"{prefix} event=sync-return v0=000000bd active=00000001 "
        "requested=000000bd remaining=000000bd dest=80078c90 "
        "expected=00008bd7 async=00000000",
        f"{prefix} event=read-return v0=000000bd active=000000bd "
        "requested=000000bd remaining=000000bd dest=80078c90 "
        "expected=00008bd7 async=00000000",
        f"{prefix} event=sync-return v0=00000001 active=00000001 "
        "requested=000000bd remaining=00000000 dest=800d7490 "
        "expected=00008c94 async=00000001",
        f"{prefix} event=sync-return v0=00000000 active=00000001 "
        "requested=000000bd remaining=00000000 dest=800d7490 "
        "expected=00008c94 async=00000000",
    ]
    if repeat:
        lines.append(lines[1])
    return "\n".join(lines) + "\n"


def selftest() -> bool:
    oracle = sample("ORACLE")
    port = sample("PORT")
    transient_pending = port.replace(
        "event=sync-return v0=00000001",
        "event=async-write v0=00000001",
        1,
    )
    passed = 0
    try:
        compare(oracle, port)
        passed += 1
        print("PASS positive: paced port matches the true-oracle completion sequence")
    except Refused as error:
        print(f"FAIL positive: {error}", file=sys.stderr)
    negatives = (
        (sample("PORT", instant=True), "instant completion before read-start return"),
        (sample("PORT", repeat=True), "same-range restart"),
        (port.replace("async=00000000", "async=00000001"), "wrong async state"),
        (transient_pending, "completion-pending state that no sync call returns"),
    )
    for changed_port, label in negatives:
        try:
            compare(oracle, changed_port)
        except Refused:
            passed += 1
            print(f"PASS negative: {label} is rejected")
        else:
            print(f"FAIL negative: {label} passed", file=sys.stderr)
    total = 1 + len(negatives)
    print(f"SELFTEST {passed}/{total}")
    return passed == total


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
        oracle, _port = compare(
            args.oracle_log.read_text(encoding="utf-8", errors="replace"),
            args.port_log.read_text(encoding="utf-8", errors="replace"),
        )
    except (OSError, Refused) as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 2
    print(
        "PASS: port matches true oracle: read-start returns 189 before completion; "
        f"callback-pending result 1 settles to 0 at sector 0x{oracle.completed.expected:X}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
