#!/usr/bin/env python3
"""psx_exe.py — the one reader for a provisioned PS-X EXE image.

WHY THIS EXISTS. Three diagnosis tools in this repository read the same file format: the
boot image (`SCUS_945.70`) plus any other PS-X EXE dumped out of a disc or a DAT. Each
implemented its own magic check and `<2I` header unpack, and they did not agree. `probe_call_sites.py`
sliced the declared text length without checking the file was that long, so a truncated dump
scanned a short word range and reported a smaller denominator over fewer bytes than the header
claimed — a quietly wrong scan with a denominator attached, which is the exact shape of bug this
repository has been bitten by twice (see `probe_addr_refs.py`'s module docstring).

One rule, one place: a PS-X EXE is `PS-X EXE` at offset 0, little-endian MIPS words, the text
address and text length live at 0x18, and the payload starts at `HEADER_BYTES`. A file whose
declared text does not fit is REFUSED — the missing bytes were not fetched and are never treated
as zero, because a scan that silently loses its upper bytes reports a confident undercount.

Raw overlays (the DAT modules) are NOT PS-X EXEs: they carry no address of their own, so they
have no header to read. Their RAM base lives in `titles/crashbash/<name>_module.json`, never in
the dump; see `probe_addr_refs.py`/`probe_call_sites.py`, which refuse to invent one.

    python3 tools/psx_exe.py <exe>
"""

from __future__ import annotations

import pathlib
import struct
from dataclasses import dataclass

MAGIC = b"PS-X EXE"
HEADER_BYTES = 0x800
ENTRY_FIELD_OFFSET = 0x10
TEXT_FIELD_OFFSET = 0x18


class Refused(Exception):
    """The file is not a usable PS-X EXE image; the caller must not scan it."""


@dataclass(frozen=True)
class PsxExeImage:
    """The declared text segment of one PS-X EXE, at the address its header names."""

    path: pathlib.Path
    entry: int
    gp: int
    text_address: int
    text: bytes

    @property
    def text_end(self) -> int:
        return self.text_address + len(self.text)


def read_image(path: pathlib.Path) -> PsxExeImage:
    """Read `path`'s text segment, or refuse — never return a short or invented image."""
    data = path.read_bytes()
    if data[: len(MAGIC)] != MAGIC:
        raise Refused(f"{path} is not a PS-X EXE (little-endian header expected)")
    entry, gp, text_address, text_size = struct.unpack_from("<4I", data, ENTRY_FIELD_OFFSET)
    text = data[HEADER_BYTES : HEADER_BYTES + text_size]
    if len(text) != text_size:
        raise Refused(
            f"{path} declares {text_size} text byte(s) and carries {len(text)}; "
            f"the missing bytes were NOT fetched and are not treated as zero"
        )
    return PsxExeImage(path, entry, gp, text_address, text)


def main() -> int:
    import sys

    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    path = pathlib.Path(sys.argv[1])
    try:
        image = read_image(path)
    except Refused as refusal:
        print(f"REFUSED: {refusal}")
        return 2
    print(
        f"{path}: entry=0x{image.entry:08X} gp=0x{image.gp:08X} "
        f"text 0x{image.text_address:08X}+{len(image.text):X} ends 0x{image.text_end:08X}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
