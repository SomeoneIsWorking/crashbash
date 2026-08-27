#!/usr/bin/env python3
"""Verify the landed CDC phase machine carries Crash Bash into later disc reads."""

from __future__ import annotations

import argparse
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
DEFAULT_LOG = ROOT / "scratch/logs/verify-cdc-phase-progress.log"
TARGET_LINE = "[cdc] sector LBA 17655 "
GET_TN = "[cdc] cmd 0x13 args=0"
COMMAND = re.compile(r"^\[cdc\] cmd 0x(?P<command>[0-9A-F]{2}) args=")
HANDLER_RESPONSE = re.compile(r"^\[cdcr\] r\[1801\]=(?P<byte>[0-9A-F]{2}).*pc=8003E14C")
IRQ_HANDLER_ENTRY = re.compile(
    r"^\[cdcr\] r\[1800\]=[0-9A-F]{2} bank=0 status pc=8003F5F0"
)
IRQ_RESPONSE = re.compile(
    r"^\[cdcr\] r\[1803\]=E(?P<type>[0-7]) bank=1 irq pc=8003E14C"
)
IRQ_ACK = re.compile(r"^\[cdcw\] w\[1803\]=07 bank=1 .*pc=8003E14C")
SECTOR = re.compile(r"^\[cdc\] sector LBA (?P<lba>[0-9]+) ")
OLD_EMPTY_POLL = "pc=8002DE2C"
EXPECTED_COMMAND_COUNTS = {0x13: 1, 0x02: 6, 0x0E: 1, 0x06: 6, 0x09: 5}
EXPECTED_AFTER_GET_TN = (0x02, 0x0E, 0x06, 0x09)
EXPECTED_RANGES = (range(35_799, 35_988), range(17_558, 17_656))
FORBIDDEN = (
    "Segmentation fault",
    "CD timeout:",
    "Cant find CRASHBSH.DAT",
    "[recomp-MISS",
    "UNHANDLED cmd",
    "FATAL",
    "[watchdog] STUCK",
)
CONTROLLER_ZERO_POSITIVE = re.compile(r"controller-zero [1-9][0-9]*")


class Refused(RuntimeError):
    """The run did not prove the declared positive CDC progress boundary."""

    def __init__(self, message: str, *, output: str = "") -> None:
        super().__init__(message)
        self.output = output


@dataclass(frozen=True)
class Verdict:
    lines: int
    response: tuple[int, ...]
    command_counts: dict[int, int]
    sectors: int
    separated_pause_responses: int


def _ordered_after(commands: list[int], start: int, expected: Sequence[int]) -> bool:
    position = start
    for command in expected:
        try:
            position = commands.index(command, position + 1)
        except ValueError:
            return False
    return True


def _pause_response_is_separated(lines: list[str]) -> bool:
    first_int3 = next(
        (
            index
            for index, line in enumerate(lines)
            if (match := IRQ_RESPONSE.match(line)) is not None
            and match.group("type") == "3"
        ),
        None,
    )
    if first_int3 is None:
        return False
    first_ack = next(
        (
            index
            for index in range(first_int3 + 1, len(lines))
            if IRQ_ACK.match(lines[index])
        ),
        None,
    )
    if first_ack is None:
        return False
    second_handler = next(
        (
            index
            for index in range(first_ack + 1, len(lines))
            if IRQ_HANDLER_ENTRY.match(lines[index])
        ),
        None,
    )
    if second_handler is None:
        return False
    second_int2 = next(
        (
            index
            for index in range(second_handler + 1, len(lines))
            if (match := IRQ_RESPONSE.match(lines[index])) is not None
            and match.group("type") == "2"
        ),
        None,
    )
    return second_int2 is not None


def judge(text: str) -> Verdict:
    lines = text.splitlines()
    forbidden = [pattern for pattern in FORBIDDEN if pattern in text]
    if CONTROLLER_ZERO_POSITIVE.search(text):
        forbidden.append("positive controller-zero fill")
    if forbidden:
        raise Refused("forbidden runtime output: " + ", ".join(forbidden))

    commands: list[int] = []
    command_lines: list[int] = []
    sectors: list[int] = []
    for index, line in enumerate(lines):
        command_match = COMMAND.match(line)
        if command_match is not None:
            commands.append(int(command_match.group("command"), 16))
            command_lines.append(index)
        sector_match = SECTOR.match(line)
        if sector_match is not None:
            sectors.append(int(sector_match.group("lba")))

    counts = {command: commands.count(command) for command in EXPECTED_COMMAND_COUNTS}
    mismatched = {
        command: (counts[command], expected)
        for command, expected in EXPECTED_COMMAND_COUNTS.items()
        if counts[command] != expected
    }
    if mismatched:
        formatted = ", ".join(
            f"0x{command:02X}={actual}/{expected}"
            for command, (actual, expected) in mismatched.items()
        )
        raise Refused(f"command denominator mismatch: {formatted}")

    get_tn_position = commands.index(0x13)
    if not _ordered_after(commands, get_tn_position, EXPECTED_AFTER_GET_TN):
        raise Refused(
            "missing ordered GetTN -> Setloc -> Setmode -> ReadN -> Pause progress"
        )

    get_tn_line = command_lines[get_tn_position]
    next_command_line = next(
        (line for line in command_lines if line > get_tn_line), len(lines)
    )
    response = tuple(
        int(match.group("byte"), 16)
        for line in lines[get_tn_line + 1 : next_command_line]
        if (match := HANDLER_RESPONSE.match(line)) is not None
    )
    if response != (0x02, 0x01, 0x01):
        raise Refused(f"GetTN response is {response}, expected (2, 1, 1)")

    pause_positions = [
        command_lines[index]
        for index, command in enumerate(commands)
        if command == 0x09
    ]
    separated_pause_responses = 0
    for pause_line in pause_positions:
        next_command_line = next(
            (line for line in command_lines if line > pause_line), len(lines)
        )
        if _pause_response_is_separated(lines[pause_line + 1 : next_command_line]):
            separated_pause_responses += 1
    expected_pauses = EXPECTED_COMMAND_COUNTS[0x09]
    if separated_pause_responses != expected_pauses:
        raise Refused(
            "Pause response-edge separation mismatch: "
            f"{separated_pause_responses}/{expected_pauses} commands expose "
            "INT3 acknowledgement and INT2 completion in distinct IRQ-handler entries"
        )

    old_polls = sum(OLD_EMPTY_POLL in line for line in lines)
    if old_polls:
        raise Refused(
            f"former 0x8002DE2C empty-poll boundary recurred {old_polls} time(s)"
        )

    sector_set = set(sectors)
    missing_sectors = [
        lba
        for expected_range in EXPECTED_RANGES
        for lba in expected_range
        if lba not in sector_set
    ]
    if missing_sectors:
        preview = ", ".join(str(lba) for lba in missing_sectors[:8])
        raise Refused(
            f"missing {len(missing_sectors)} required sector event(s), first: {preview}"
        )
    if TARGET_LINE not in text:
        raise Refused("positive target LBA 17655 was not observed")

    return Verdict(
        len(lines), response, counts, len(sectors), separated_pause_responses
    )


def _reader(stream, output: queue.Queue[str | None]) -> None:
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


def run_until_target(
    arguments: Sequence[str], *, cwd: Path, environment: dict[str, str], timeout: float
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
    target_seen = False
    failure: str | None = None
    deadline = time.monotonic() + timeout
    try:
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                failure = f"port PID {process.pid} did not reach positive target LBA 17655 in {timeout}s"
                break
            try:
                item = output.get(timeout=min(remaining, 0.1))
            except queue.Empty:
                if process.poll() is not None and not reader.is_alive():
                    failure = (
                        f"port PID {process.pid} exited with {process.returncode} "
                        "before positive target LBA 17655"
                    )
                    break
                continue
            if item is None:
                failure = (
                    f"port PID {process.pid} exited with {process.returncode} "
                    "before positive target LBA 17655"
                )
                break
            lines.append(item)
            if TARGET_LINE in item:
                target_seen = True
                break
    finally:
        _stop_exact_child(process)
        reader.join(timeout=2.0)
        while True:
            try:
                item = output.get_nowait()
            except queue.Empty:
                break
            if item is not None:
                lines.append(item)

    text = "".join(lines)
    if failure is not None or not target_seen:
        raise Refused(
            failure or "positive target LBA 17655 was not observed", output=text
        )
    return text


def run(port: Path, executable: Path, timeout: float) -> str:
    if not port.is_file() or not executable.is_file():
        raise Refused(f"port or retail executable absent: {port}, {executable}")
    environment = dict(os.environ)
    environment.update(
        PSXPORT_ASSET_DIR=str(ROOT / "external/psxport"),
        PSXPORT_VK_HEADLESS="1",
        PSXPORT_NOPACE="1",
        PSXPORT_NOAUDIO="1",
        PSXPORT_DEBUG="cdc,cdcpace,cdcr,cdcw",
        PSXPORT_FNTRACE="8002DE2C",
        PSXPORT_WATCHDOG="2",
    )
    DEFAULT_LOG.parent.mkdir(parents=True, exist_ok=True)
    try:
        text = run_until_target(
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


def _fixture() -> str:
    lines = [
        GET_TN,
        "[cdcr] r[1801]=02 bank=1 resp pc=8003E14C ra=8003F62C",
        "[cdcr] r[1801]=01 bank=1 resp pc=8003E14C ra=8003F62C",
        "[cdcr] r[1801]=01 bank=1 resp pc=8003E14C ra=8003F62C",
    ]
    command_counts = ((0x02, 6), (0x0E, 1), (0x06, 6), (0x09, 5))
    for command, count in command_counts:
        for _ in range(count):
            lines.append(f"[cdc] cmd 0x{command:02X} args=0")
            if command == 0x09:
                lines.extend(
                    (
                        "[cdcr] r[1800]=78 bank=0 status pc=8003F5F0 ra=80031C34",
                        "[cdcr] r[1803]=E3 bank=1 irq pc=8003E14C ra=8003F62C",
                        "[cdcw] w[1803]=07 bank=1 a0=80068AAC s1=00000000 pc=8003E14C ra=8003F62C",
                        "[cdcr] r[1800]=78 bank=0 status pc=8003F5F0 ra=80031C34",
                        "[cdcr] r[1803]=E2 bank=1 irq pc=8003E14C ra=8003F62C",
                        "[cdcw] w[1803]=07 bank=1 a0=80068AAC s1=00000000 pc=8003E14C ra=8003F62C",
                    )
                )
    for expected_range in EXPECTED_RANGES:
        lines.extend(
            f"[cdc] sector LBA {lba} file=0 chan=0 submode=0x08 audio=0 -> data FIFO"
            for lba in expected_range
        )
    return "\n".join(lines) + "\n"


def selftest() -> bool:
    fixture = _fixture()
    passed = 0
    cases = 8
    try:
        verdict = judge(fixture)
        passed += 1
        print(
            f"PASS positive: {verdict.lines} lines, {verdict.sectors} sector events, "
            f"GetTN response {verdict.response}"
        )
    except Refused as error:
        print(f"FAIL positive: {error}", file=sys.stderr)

    negatives = (
        (
            fixture.replace(TARGET_LINE, "[cdc] sector LBA 17654 "),
            "missing positive target",
        ),
        (fixture + "[cdcr] r[1803]=E0 bank=1 irq pc=8002DE2C\n", "old empty poll"),
        (fixture.replace("r[1801]=01", "r[1801]=00", 1), "wrong GetTN response"),
        (fixture.replace("[cdc] cmd 0x09 args=0\n", "", 1), "command denominator"),
        (fixture + "[recomp-MISS forced-negative]\n", "forbidden runtime error"),
        (
            fixture.replace(
                "[cdcw] w[1803]=07 bank=1 a0=80068AAC s1=00000000 pc=8003E14C ra=8003F62C\n"
                "[cdcr] r[1800]=78 bank=0 status pc=8003F5F0 ra=80031C34\n"
                "[cdcr] r[1803]=E2 bank=1 irq pc=8003E14C ra=8003F62C\n",
                "[cdcw] w[1803]=07 bank=1 a0=80068AAC s1=00000000 pc=8003E14C ra=8003F62C\n"
                "[cdcr] r[1803]=E2 bank=1 irq pc=8003E14C ra=8003F62C\n",
                1,
            ),
            "Pause completion coalesced without a second IRQ-handler entry",
        ),
    )
    for changed, label in negatives:
        try:
            judge(changed)
        except Refused:
            passed += 1
            print(f"PASS negative: {label} is rejected")
        else:
            print(f"FAIL negative: {label} passed", file=sys.stderr)

    with tempfile.TemporaryDirectory(dir=ROOT / "scratch") as directory:
        timed_out = Path(directory) / "timed-out.py"
        timed_out.write_text(
            "import time\nprint('started', flush=True)\ntime.sleep(5)\n"
        )
        try:
            run_until_target(
                [sys.executable, str(timed_out)],
                cwd=ROOT,
                environment=dict(os.environ),
                timeout=0.05,
            )
        except Refused as error:
            if "started" in error.output:
                passed += 1
                print("PASS negative: timeout refuses and preserves child output")
            else:
                print("FAIL negative: timeout discarded child output", file=sys.stderr)
        else:
            print("FAIL negative: timeout passed", file=sys.stderr)

    print(f"SELFTEST {passed}/{cases}")
    return passed == cases


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=Path, default=DEFAULT_PORT)
    parser.add_argument("--executable", type=Path, default=DEFAULT_EXECUTABLE)
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--trace", type=Path)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    try:
        if args.selftest:
            return 0 if selftest() else 1
        if args.trace is not None:
            text = args.trace.read_text(encoding="utf-8", errors="replace")
        else:
            text = run(args.port, args.executable, args.timeout)
        verdict = judge(text)
    except (OSError, Refused) as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 2
    print(
        f"PASS: {verdict.lines} lines; GetTN {verdict.response}; "
        f"commands {verdict.command_counts}; {verdict.sectors} sector events through LBA 17655; "
        f"{verdict.separated_pause_responses} Pause INT3/INT2 response pairs separated"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
