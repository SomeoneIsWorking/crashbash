#!/usr/bin/env python3
"""probe_call_sites.py — diagnosis only: who calls a guest address, and what does it call.

A pure-word MIPS scanner, not a disassembler: `jal` is opcode 0b000011 and its
target is ((delay-slot PC & 0xF0000000) | ((word & 0x03FFFFFF) << 2)), so a caller
census needs no third-party
decoder and cannot silently mis-decode a whole image. Every answer carries its
denominator (words scanned, `jal` decoded, matches) so a zero result reads as
"scanned N, matched 0" instead of "the tool found nothing".

  # call sites of one address, in every image
  python3 tools/probe_call_sites.py --root scratch/bin/crashbash --callee 0x800189F4

  # what one function calls, in every image
  python3 tools/probe_call_sites.py --root scratch/bin/crashbash --caller 0x8001965C

  # one image only
  python3 tools/probe_call_sites.py --root scratch/bin/crashbash --image MENU --callee 0x800189F4

  # who HOLDS the address: every 32-bit word equal to it (a patched vtable slot, a pointer
  # global, a jump-table entry). This game dispatches through tables, so a jal census can read
  # "matched 0" for a function that is very much called.
  python3 tools/probe_call_sites.py --root scratch/bin/crashbash --word 0x800189F4
"""

from __future__ import annotations

import argparse
import pathlib
import struct
from dataclasses import dataclass

import psx_exe

JAL_OPCODE = 0x03
OPCODE_SHIFT = 26
OPCODE_MASK = 0x3F
TARGET_MASK = 0x03FFFFFF


@dataclass(frozen=True)
class Image:
    name: str
    path: pathlib.Path
    base: int
    payload: bytes

    @property
    def end(self) -> int:
        return self.base + len(self.payload)


def psx_exe_image(name: str, path: pathlib.Path) -> Image:
    try:
        image = psx_exe.read_image(path)
    except psx_exe.Refused as refusal:
        raise SystemExit(f"REFUSED: {refusal}") from refusal
    return Image(name, path, image.text_address, image.text)


def raw_image(name: str, path: pathlib.Path, base: int) -> Image:
    return Image(name, path, base, path.read_bytes())


def load_images(root: pathlib.Path, only: str | None, overlay_base: int) -> list[Image]:
    images: list[Image] = []
    exe = root / "SCUS_945.70"
    if exe.is_file():
        images.append(psx_exe_image("SCUS_945.70", exe))
    overlays = root / "overlays"
    if overlays.is_dir():
        # An overlay is a raw payload with its RAM base recorded in titles/crashbash/<name>_module.json,
        # never in the dump, so a scan without --overlay-base reports a denominator and refuses to
        # invent addresses rather than printing confident nonsense.
        for path in sorted(overlays.glob("*.BIN")):
            images.append(raw_image(path.stem, path, overlay_base))
    if only is not None:
        images = [image for image in images if image.name.upper() == only.upper()]
        if not images:
            raise SystemExit(f"no image named {only} under {root}")
    if not images:
        raise SystemExit(f"no PS-X EXE or overlay under {root}")
    return images


def jump_target(site: int, word: int) -> int:
    """A `jal`'s 26-bit field is an OFFSET, not an address: the top four bits come from the delay slot.

    MIPS `J`/`JAL` forms `target = (PC_of_delay_slot & 0xF0000000) | (imm26 << 2)`. The delay slot is the
    instruction AFTER the jump, so the region is taken from `site + 4`.

    **Dropping the region term caps every target at 0x0FFFFFFC**, so `--callee 0x800272AC` -- any real
    guest address -- could never match and the tool printed `matched 0` over a full scan. That is the same
    failure as arming the store observer on a data address: a query guaranteed to come back empty, printed
    with a denominator, and readable as a finding. It was found on 2026-09-27 while establishing Crash
    Bash's retail cadence (docs/issues/0031), and the tool had NO `--selftest`, which is why it survived.
    """
    return ((site + 4) & 0xF0000000) | ((word & TARGET_MASK) << 2)


def scan(image: Image) -> tuple[list[tuple[int, int]], int]:
    """Return (calls, words) where each call is (site, target)."""
    words = len(image.payload) // 4
    calls: list[tuple[int, int]] = []
    for index in range(words):
        word = struct.unpack_from("<I", image.payload, index * 4)[0]
        if (word >> OPCODE_SHIFT) & OPCODE_MASK == JAL_OPCODE:
            site = image.base + index * 4
            calls.append((site, jump_target(site, word)))
    return calls, words


def selftest() -> int:
    """The JAL region term, on a real instruction from the provisioned image.

    Every case here is checkable without a disc, a product or a product slot, which is the point: the
    defect this covers shipped and survived because the tool had no way to be wrong out loud. The first
    case is the instruction that exposed it -- `jal 0x800320EC` encoded at 0x80027388 as 0x0C00C83B -- and
    the second asserts the property that made the old answer undetectable, which is that the resolved
    target is unreachable without the region term.
    """
    failures: list[str] = []
    checks = 0

    def expect(condition: bool, label: str) -> None:
        nonlocal checks
        checks += 1
        if not condition:
            failures.append(label)

    # The real instruction: `80027388  jal 0x800320EC`, word 0x0C00C83B, delay slot at 0x8002738C.
    expect(jump_target(0x80027388, 0x0C00C83B) == 0x800320EC,
           f"jal at 0x80027388 must resolve to 0x800320EC, got 0x{jump_target(0x80027388, 0x0C00C83B):08X}")

    # THE TRAP, stated as a property: without the region term no target can exceed 0x0FFFFFFC, so every
    # real guest address is unreachable. If this assertion ever fails because the term came back, the
    # fix has been reverted and `--callee` is guaranteed to print `matched 0` again.
    expect((0x03FFFFFF << 2) < 0x80000000,
           "a region-less target is bounded by 0x0FFFFFFC and cannot reach guest addresses")
    for site, word in ((0x80027388, 0x0C00C83B), (0x80011BF8, 0x0C0046FE), (0x8001018C, 0x0C0046FE)):
        expect(jump_target(site, word) >= 0x80000000,
               f"jal at 0x{site:08X} resolved below the load region: 0x{jump_target(site, word):08X}")

    # A jump that crosses no region boundary keeps its offset, and the low bits are the offset.
    expect(jump_target(0x80027388, 0x0C00C83B) & 0x3 == 0, "a jal target is 4-byte aligned")

    # Low-RAM code: the region term must come from the PC, not be assumed to be 0x80000000. A function at
    # 0x00020000 jumping must stay in low RAM.
    expect(jump_target(0x00020000, 0x0C00C83B) == 0x000320EC,
           f"low-RAM jal must keep the low region, got 0x{jump_target(0x00020000, 0x0C00C83B):08X}")

    # The field is an OFFSET from the region, so the same encoding resolves to a different address
    # depending on where the jump sits. The region is the top FOUR BITS -- one hex digit, `& 0xF0000000`
    # -- which is the detail I got wrong writing this test the first time: 0x84.. and 0x8C.. are BOTH
    # region 0x8, so neither moves, and only 0xC0.. crosses. Both cases are asserted because the wrong
    # one was a wrong expectation rather than a wrong function.
    for same_region in (0x84000000, 0x8C000000):
        expect(jump_target(same_region, 0x0C00C83B) == jump_target(0x80000000, 0x0C00C83B),
               f"0x{same_region:08X} shares the 0x8 region, so the target must not move")
    a = jump_target(0x80000000, 0x0C00C83B)
    b = jump_target(0xC0000000, 0x0C00C83B)
    expect((b - a) == 0x40000000,
           f"the region must come from the PC: 0x{a:08X} vs 0x{b:08X}")
    expect(b == 0xC00320EC, f"0xC0.. must resolve into the 0xC region, got 0x{b:08X}")

    # A JAL in the last four bytes of a region supplies the NEXT region's nibble, because the region comes
    # from the DELAY SLOT's PC, which is the instruction after the jump. Real hardware does this and so must
    # this: a region-less formula cannot express it either way.
    edge = jump_target(0x8FFFFFFC, 0x0C00C83B)
    expect(edge == 0x900320EC,
           f"a JAL whose delay slot crosses into 0x9 must target 0x9, got 0x{edge:08X}")

    if failures:
        print(f"[probe] selftest FAIL: {len(failures)} of {checks} checks failed")
        for label in failures:
            print(f"[probe]   - {label}")
        return 1
    print(f"[probe] selftest OK: {checks} checks -- a jal target is (delay-slot PC & 0xF0000000) | (imm26 << 2), "
          f"so every guest address is reachable and none is reachable without the region term")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true",
                        help="validate the jal target decode on real instructions; no disc, no product")
    parser.add_argument(
        "--root",
        type=pathlib.Path,
        help="directory holding SCUS_945.70 and overlays/ (required for a scan; --selftest needs no "
             "disc, no product and no root)",
    )
    parser.add_argument(
        "--image", help="restrict to one image (SCUS_945.70, MENU, ...)"
    )
    parser.add_argument(
        "--callee",
        type=lambda value: int(value, 0),
        help="report every jal TO this address",
    )
    parser.add_argument(
        "--caller",
        type=lambda value: int(value, 0),
        help="report every jal FROM this address",
    )
    parser.add_argument(
        "--word",
        type=lambda value: int(value, 0),
        help="report every 32-bit word EQUAL to this value (pointer / vtable slot)",
    )
    parser.add_argument(
        "--overlay-base",
        type=lambda value: int(value, 0),
        default=0,
        help="base address for raw overlay scans (default 0)",
    )
    arguments = parser.parse_args()
    # The decoder is checkable with no disc, no product and no root, so it is checkable ALWAYS. Before
    # this flag existed the only way to run the tool needed a provisioned image, and the JAL region bug
    # below survived precisely because nothing could exercise it without one.
    if arguments.selftest:
        return selftest()
    # A scan with no corpus must REFUSE rather than scan nothing and print a total that reads as a
    # finding. argparse used to enforce this, which also blocked --selftest; the guarantee is kept here.
    if arguments.root is None:
        print("[probe] REFUSED: --root is required for a scan; nothing was scanned. Pass the directory "
              "holding SCUS_945.70 and overlays/, or use --selftest to validate the decoder alone.")
        return 2
    selectors = [
        name
        for name in ("callee", "caller", "word")
        if getattr(arguments, name) is not None
    ]
    if len(selectors) != 1:
        # A selector is a QUESTION. With none, or with several, there is no single answer to report, and
        # a tool that guessed would print a total that reads as a finding. Raise, do not default.
        raise SystemExit("give exactly one of --callee / --caller / --word (or --selftest, which needs "
                         "neither)")

    grand_words = 0
    grand_calls = 0
    grand_hits = 0
    for image in load_images(arguments.root, arguments.image, arguments.overlay_base):
        words = len(image.payload) // 4
        if arguments.word is not None:
            needle = struct.pack("<I", arguments.word)
            hits = []
            position = image.payload.find(needle)
            while position != -1 and position % 4 == 0:
                hits.append(image.base + position)
                position = image.payload.find(needle, position + 4)
            calls: list[tuple[int, int]] = []
        else:
            calls, _ = scan(image)
            grand_calls += len(calls)
            if arguments.callee is not None:
                hits = [site for site, target in calls if target == arguments.callee]
            else:
                hits = [target for site, target in calls if site == arguments.caller]
        grand_words += words
        grand_hits += len(hits)
        print(
            f"[probe] {image.name}: scanned {words} word(s) at 0x{image.base:08X}..0x{image.end:08X}, "
            f"decoded {len(calls)} jal, matched {len(hits)}"
        )
        for value in hits:
            print(f"    0x{value:08X}")
    print(
        f"[probe] TOTAL: scanned {grand_words} word(s) of 1+ image(s), decoded {grand_calls} jal, matched {grand_hits}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
