#!/usr/bin/env python3
"""verify_bounded_resume.py — read a Crash Bash run log and answer the bounded-resume question.

WHAT IT ANSWERS, from the log the product already wrote, with a denominator for every number:
  frames      the highest `f<N>` the run reached, and whether it reached the requested frame cap
  fences      presentation commits, split into real presents and unpresented frames
  fields      guest VBlank fields delivered, and the last VBlank counter value
  resumes     guest calls that outlived one display field, the deepest one, and their turn cap
  executor    `executor:error` lines and SIGABRT, which is what the pre-fix tree died of
  census      the run's own guest-call denominator, so "nothing needed a resume" is a claim with a
              count behind it rather than silence

It REFUSES (exit 2) on a missing, unreadable, or empty log instead of reporting a green tick, and it
prints how many lines it scanned so a zero match is "scanned N, matched 0" and not "found nothing".

    python3 tools/verify_bounded_resume.py scratch/afterfix/run.log
    python3 tools/verify_bounded_resume.py run.log --expect-frames 400

`--selftest` runs the three cases that matter against synthetic logs — a completed run, a run that
died on an `executor:error`, and a log that is not there — and asserts the tool REFUSES or FAILS
rather than reporting a green tick for each. A verifier nothing runs is the surface nobody calls.
"""

from __future__ import annotations

import argparse
import contextlib
import io
import pathlib
import re
import sys
import tempfile

FRAME = re.compile(r"crashbash-frame\] f(\d+):")
APP_MODE = re.compile(r"crashbash-frame\] f(\d+): app mode -> (0x[0-9A-F]+)")
PROCESS_STATE = re.compile(
    r"crashbash-frame\] process state -> (0x[0-9A-F]+) \(entry #(\d+)"
)
FIELDS_DELIVERED = re.compile(r"(\d+) field\(s\) delivered")
VBLANK = re.compile(r"vblank counter (0x[0-9A-F]+)")
EXECUTOR_ERROR = re.compile(r"\[executor:error\]")
FATAL = re.compile(r"FATAL|SIGABRT|Aborted")
RESUMED_CALL = re.compile(
    r"guest call (0x[0-9A-F]+) to return address (0x[0-9A-F]+) outlived one host turn: (\d+) turn\(s\), "
    r"(\d+) guest cycles \(([0-9.]+) display fields\)"
)
CALL_CENSUS = re.compile(
    r"run-end \(([^)]*)\): (\d+) guest call\(s\) completed, (\d+) needed a resume"
)
NO_CALLS = re.compile(r"run-end \(([^)]*)\): NO guest call completed")
LOOP_DONE = re.compile(r"native_boot\] frame loop done")
TURN_CAP = re.compile(r"deepest (\d+) host turn\(s\) against a cap of (\d+)")
MENU_ENTRY = re.compile(
    r"crashbash-boundary\] MENU entry addr=([0-9A-F]+) ra=([0-9A-F]+)"
)

PASS, FAIL, REFUSED = 0, 1, 2

COMPLETED_RUN = """\
[2026-01-01T00:00:00.000Z] [crashbash-frame] process state -> 0x8004E0B8 (entry #1, enter=0x80010410 update=0x80010394 present=0x80010278)
[2026-01-01T00:00:00.000Z] [crashbash-frame] f0: app mode -> 0x80078C90 (change #1, enter=0x80092BDC update=0x80092BA0 present=0x80092B7C)
[2026-01-01T00:00:00.000Z] [crashbash-frame] f1: dwelling in state 0x8004E0B8 for 2 frame(s) (update=0x80010394 present=0x80010278, 2 field(s) delivered, vblank counter 0x00000005, app mode 0x80078C90 unchanged for the whole dwell)
[2026-01-01T00:00:00.000Z] [crashbash-boundary] MENU entry addr=800B5244 ra=8001E7C0
[2026-01-01T00:00:01.000Z] [crashbash-guest] guest call 0x800B5244 to return address 0x8001E7C0 outlived one host turn: 6 turn(s), 3135214 guest cycles (5.556 display fields). Denominator: 1 of 1 completed guest call(s) have needed a resume; deepest 6 turn(s)
[2026-01-01T00:00:02.000Z] [native_boot] frame loop done
[2026-01-01T00:00:02.000Z] [crashbash-guest] run-end (after native boot): 1 guest call(s) completed, 1 needed a resume (deepest 6 host turn(s) against a cap of 8, 3135214 guest cycles over those calls); the other 0 finished inside one field
"""

DIED_RUN = """\
[2026-01-01T00:00:00.000Z] [crashbash-frame] f0: app mode -> 0x80078C90 (change #1, enter=0x80092BDC update=0x80092BA0 present=0x80092B7C)
[2026-01-01T00:00:00.000Z] [crashbash-frame] f7: dwelling in state 0x8004E0B8 for 8 frame(s) (update=0x80010394 present=0x80010278, 0 field(s) delivered, vblank counter 0x00000009, app mode 0x80078C90 unchanged for the whole dwell)
[2026-01-01T00:00:01.000Z] [executor:error] Crash Bash original call required a completed guest call, but execution exited as budget-exhausted at 0x80018AA0 after 564500 cycles: cycle budget exhausted
"""

TRUNCATED_RUN = """\
[2026-01-01T00:00:00.000Z] [crashbash-frame] f0: app mode -> 0x80078C90 (change #1, enter=0x80092BDC update=0x80092BA0 present=0x80092B7C)
"""


def selftest() -> int:
    checks = 0
    failures = 0
    with tempfile.TemporaryDirectory(prefix="crashbash-verify-") as directory:
        root = pathlib.Path(directory)
        cases = [
            ("completed.log", COMPLETED_RUN, ["--expect-frames", "2"], PASS),
            ("died.log", DIED_RUN, ["--expect-frames", "400"], FAIL),
            ("truncated.log", TRUNCATED_RUN, [], FAIL),
            ("empty.log", "", [], REFUSED),
            ("missing.log", None, [], REFUSED),
        ]
        for name, content, extra, expected in cases:
            path = root / name
            if content is not None:
                path.write_text(content)
            buffer = io.StringIO()
            with contextlib.redirect_stdout(buffer):
                code = main_for(path, extra)
            checks += 1
            if code != expected:
                failures += 1
                print(f"    FAIL {name}: expected exit {expected}, got {code}")
    print(f"[selftest] {checks - failures}/{checks} case(s) behaved as required")
    return 0 if failures == 0 else 1


def main() -> int:
    if "--selftest" in sys.argv[1:]:
        return selftest()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=pathlib.Path, help="the run log the product wrote")
    parser.add_argument(
        "--expect-frames",
        type=int,
        default=0,
        help="frame cap the run was asked for; 0 skips the reaches check",
    )
    arguments = parser.parse_args()
    return verify(arguments.log, arguments.expect_frames)


def main_for(path: pathlib.Path, extra: list[str]) -> int:
    return verify(path, int(extra[1]) if len(extra) > 1 else 0)


def verify(path: pathlib.Path, expect_frames: int) -> int:
    arguments = argparse.Namespace(log=path, expect_frames=expect_frames)

    if not arguments.log.is_file():
        print(f"[verify] REFUSED: {arguments.log} is not a readable file")
        return REFUSED
    lines = arguments.log.read_text(errors="replace").splitlines()
    if not lines:
        print(f"[verify] REFUSED: {arguments.log} is empty")
        return REFUSED

    frames = [
        int(match.group(1)) for line in lines for match in [FRAME.search(line)] if match
    ]
    modes = [
        (int(m.group(1)), m.group(2))
        for line in lines
        for m in [APP_MODE.search(line)]
        if m
    ]
    states = [
        (m.group(1), int(m.group(2)))
        for line in lines
        for m in [PROCESS_STATE.search(line)]
        if m
    ]
    delivered = [
        int(m.group(1)) for line in lines for m in [FIELDS_DELIVERED.search(line)] if m
    ]
    vblanks = [m.group(1) for line in lines for m in [VBLANK.search(line)] if m]
    executor_errors = [line for line in lines if EXECUTOR_ERROR.search(line)]
    fatals = [line for line in lines if FATAL.search(line)]
    resumes = [m.groups() for line in lines for m in [RESUMED_CALL.search(line)] if m]
    censuses = [m.groups() for line in lines for m in [CALL_CENSUS.search(line)] if m]
    no_calls = [m.group(1) for line in lines for m in [NO_CALLS.search(line)] if m]
    caps = [m.groups() for line in lines for m in [TURN_CAP.search(line)] if m]
    finished = sum(1 for line in lines if LOOP_DONE.search(line))
    menu_entries = [
        m.groups() for line in lines for m in [MENU_ENTRY.search(line)] if m
    ]

    print(f"[verify] scanned {len(lines)} line(s) of {arguments.log}")
    print(
        f"[verify] frames: reached f{max(frames) if frames else 0}, {len(frames)} frame progress line(s), "
        f"{finished} 'frame loop done' line(s)"
    )
    if arguments.expect_frames:
        reached = (
            arguments.expect_frames if finished else (max(frames) + 1 if frames else 0)
        )
        print(
            f"[verify] reaches: {reached} of {arguments.expect_frames} requested frame(s) "
            f"({'frame loop done' if finished else 'log has NO frame-loop-done line'}); "
            f"highest reported progress line f{max(frames) if frames else 0}"
        )
    print(
        f"[verify] process state: {len(states)} entr(y/ies) {sorted({state for state, _ in states})}"
    )
    print(
        f"[verify] app mode: {len(modes)} change(s) {sorted({mode for _, mode in modes})}"
    )
    if delivered:
        print(
            f"[verify] fields delivered: {sum(delivered)} over {len(delivered)} reported frame(s); "
            f"last vblank counter {vblanks[-1] if vblanks else 'n/a'}"
        )
    print(
        f"[verify] presentation fences: {max(frames) + 1 if frames else 0} frame(s) committed; "
        f"{sum(1 for fields in delivered if fields == 0)} reported frame(s) delivered 0 field(s)"
    )
    print(f"[verify] guest calls that outlived one field: {len(resumes)}")
    for entry, return_pc, turns, cycles, fields in resumes:
        print(
            f"           {entry} -> {return_pc}: {turns} turn(s), {cycles} cycles, {fields} fields"
        )
    if caps:
        print(
            f"[verify] run census: deepest {caps[-1][0]} turn(s) against a cap of {caps[-1][1]}"
        )
    for why, completed, resumed in censuses:
        print(
            f"[verify] guest-call census ({why}): {completed} completed, {resumed} needed a resume"
        )
    for why in no_calls:
        print(
            f"[verify] guest-call census ({why}): NONE completed — this run measured no guest call"
        )
    for entry, return_pc in menu_entries:
        print(f"[verify] menu entry observed: {entry} ra={return_pc}")
    print(
        f"[verify] executor:error lines: {len(executor_errors)} of {len(lines)} scanned"
    )
    for line in executor_errors:
        print(f"           {line.strip()}")
    print(f"[verify] fatal/abort lines: {len(fatals)}")

    if executor_errors or fatals:
        print("[verify] FAIL: the run reported an executor error or aborted")
        return FAIL
    if (
        arguments.expect_frames
        and not finished
        and (max(frames) + 1 if frames else 0) < arguments.expect_frames
    ):
        reached = max(frames) + 1 if frames else 0
        print(
            f"[verify] FAIL: reached {reached} of {arguments.expect_frames} frame(s), and the log has no "
            f"'frame loop done' line, so the run ended early without saying why"
        )
        return FAIL
    if not arguments.expect_frames and not finished:
        print(
            "[verify] INCOMPLETE: the log has no 'frame loop done' line, so the run ended early "
            "without saying why; treat the numbers above as partial"
        )
        return FAIL
    print(
        "[verify] PASS: no executor error, no abort, and the frame count asked for was reached"
    )
    return PASS


if __name__ == "__main__":
    raise SystemExit(main())
