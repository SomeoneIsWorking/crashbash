#!/usr/bin/env python3
"""Verify a live rtpcaller trace attributes presented projection work to exact anchors.

The shipping framework's ``PSXPORT_DEBUG=rtpcaller`` instrument emits one histogram every 50
presented frames. Each row decodes the guest ``jal`` at RA-8 and reports its target function. This
judge binds those targets to the provenance-checked generated inventory; it does not infer a camera,
primitive semantics, or a native producer from reaching an address.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

import inventory_render_anchors
import recomp_bootstrap

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LOG = ROOT / "scratch/logs/crashbash-render-anchor-reach.log"
HISTOGRAM = re.compile(
    r"\[rtpcaller\]\s+f(?P<frame>[1-9][0-9]*)\(last50\) RA histogram"
)
ROW = re.compile(
    r"\[rtpcaller\]\s+RA=0x(?P<ra>[0-9A-Fa-f]{8})\s+"
    r"(?P<count>[1-9][0-9]*)\s+jal\[(?P<jal>[0-9A-Fa-f]{8})\]"
    r"->fn=0x(?P<target>[0-9A-Fa-f]{8})"
)
FORBIDDEN = (
    "[recomp-MISS",
    "FATAL",
    "Segmentation fault",
    "[watchdog] STUCK",
)


class Refused(RuntimeError):
    """The trace did not prove exact runtime projection-anchor attribution."""


@dataclass(frozen=True)
class Verdict:
    frames: tuple[int, ...]
    rows: int
    calls: int
    reached_anchors: tuple[int, ...]


def projection_anchors(inventory: dict[str, object]) -> set[int]:
    anchors = inventory.get("anchors")
    if not isinstance(anchors, list):
        raise Refused("static inventory has no projection-anchor list")
    try:
        return {int(str(anchor["address"]), 16) for anchor in anchors}
    except (KeyError, TypeError, ValueError) as error:
        raise Refused(
            "static inventory contains an invalid projection address"
        ) from error


def judge(text: str, anchors: set[int]) -> Verdict:
    forbidden = [pattern for pattern in FORBIDDEN if pattern in text]
    if forbidden:
        raise Refused("forbidden runtime output: " + ", ".join(forbidden))
    frames = tuple(int(match.group("frame")) for match in HISTOGRAM.finditer(text))
    if not frames:
        raise Refused("no completed rtpcaller 50-frame histogram was observed")
    rows = list(ROW.finditer(text))
    if not rows:
        raise Refused("rtpcaller histogram contains no projection-call rows")

    zero_targets = sum(int(match.group("target"), 16) == 0 for match in rows)
    if zero_targets:
        raise Refused(
            f"{zero_targets} rtpcaller row(s) came from jalr/inlined ancestry and remain unattributed"
        )
    targets = {int(match.group("target"), 16) for match in rows}
    unknown = sorted(targets - anchors)
    if unknown:
        rendered = ", ".join(f"0x{address:08X}" for address in unknown)
        raise Refused(
            "runtime projection target(s) are absent from the exact generated inventory: "
            + rendered
        )
    return Verdict(
        frames=frames,
        rows=len(rows),
        calls=sum(int(match.group("count")) for match in rows),
        reached_anchors=tuple(sorted(targets)),
    )


def run_selftest() -> None:
    anchors = {0x8001CD04, 0x800193A8}
    positive = """
[rtpcaller] f50(last50) RA histogram (caller return site -> jal target = projection fn):
[rtpcaller]     RA=0x800157A0        12   jal[0C007341]->fn=0x8001CD04
[rtpcaller]     RA=0x8001CD24         7   jal[0C0064EA]->fn=0x800193A8
"""
    verdict = judge(positive, anchors)
    assert verdict.frames == (50,)
    assert verdict.rows == 2
    assert verdict.calls == 19
    assert verdict.reached_anchors == (0x800193A8, 0x8001CD04)

    negatives = (
        positive.replace("0x800193A8", "0x8001AF2C"),
        positive.replace("0x800193A8", "0x00000000"),
        "[rtpcaller] f50(last50) RA histogram\n",
        positive + "[recomp-MISS 0x80000000]\n",
    )
    for fixture in negatives:
        try:
            judge(fixture, anchors)
        except Refused:
            continue
        raise AssertionError("render-anchor reach negative was accepted")
    print("render-anchor reach selftest: 8/8 checks passed")


def exact_inventory() -> dict[str, object]:
    generated = ROOT / "generated"
    provenance = inventory_render_anchors.load_provenance(generated)
    return inventory_render_anchors.build_inventory(
        inventory_render_anchors.load_functions(generated), provenance
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", nargs="?", type=Path, default=DEFAULT_LOG)
    parser.add_argument("--selftest", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.selftest:
        run_selftest()
        return 0
    try:
        text = args.log.read_text(encoding="utf-8", errors="replace")
        inventory = exact_inventory()
        verdict = judge(text, projection_anchors(inventory))
    except (OSError, TypeError, ValueError, Refused, recomp_bootstrap.Refused) as error:
        print(f"render-anchor reach: REFUSED: {error}", file=sys.stderr)
        return 2
    provenance = inventory["provenance"]
    assert isinstance(provenance, dict)
    anchors = ", ".join(f"0x{address:08X}" for address in verdict.reached_anchors)
    print(
        "render-anchor reach: PASS: "
        f"psxport {provenance['psxport_commit']} / recompiler "
        f"{provenance['recompiler_version']}; {len(verdict.frames)} histogram(s), "
        f"{verdict.rows} row(s), {verdict.calls} call(s), anchors {anchors}"
    )
    print(
        "LIMIT: reach proves runtime projection ancestry only; camera semantics, scene state, "
        "primitive ownership, 4:3 parity, widescreen, and interpolation remain unproven"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
