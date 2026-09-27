#!/usr/bin/env python3
"""probe_call_sites.py — diagnosis only: who calls a guest address, and what does it call.

A pure-word MIPS scanner, not a disassembler: `jal` is opcode 0b000011 and its
target is ((word & 0x03FFFFFF) << 2), so a caller census needs no third-party
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

PSX_EXE_HEADER_BYTES = 0x800
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
    data = path.read_bytes()
    if data[:8] != b"PS-X EXE":
        raise SystemExit(f"{path} is not a PS-X EXE")
    t_addr, t_size = struct.unpack_from("<2I", data, 0x18)
    return Image(
        name, path, t_addr, data[PSX_EXE_HEADER_BYTES : PSX_EXE_HEADER_BYTES + t_size]
    )


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


def scan(image: Image) -> tuple[list[tuple[int, int]], int]:
    """Return (calls, words) where each call is (site, target)."""
    words = len(image.payload) // 4
    calls: list[tuple[int, int]] = []
    for index in range(words):
        word = struct.unpack_from("<I", image.payload, index * 4)[0]
        if (word >> OPCODE_SHIFT) & OPCODE_MASK == JAL_OPCODE:
            calls.append((image.base + index * 4, (word & TARGET_MASK) << 2))
    return calls, words


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=pathlib.Path,
        required=True,
        help="directory holding SCUS_945.70 and overlays/",
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
    selectors = [
        name
        for name in ("callee", "caller", "word")
        if getattr(arguments, name) is not None
    ]
    if len(selectors) != 1:
        raise SystemExit("give exactly one of --callee / --caller / --word")

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
