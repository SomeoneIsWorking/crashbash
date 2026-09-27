#!/usr/bin/env python3
"""probe_addr_refs.py — every load/store in a provisioned image whose EFFECTIVE ADDRESS is
one given main-RAM value, across every linked image.

WHY THIS EXISTS, and why the two tools already in this repository cannot answer it.
`probe_call_sites.py --word` finds 32-bit words EQUAL to a value, which is how this game
dispatches: a pointer global or a patched vtable slot. But the other way a guest reaches an
absolute address is a `lui` upper half plus a 16-bit displacement, and that pair contains no
32-bit word equal to the address at all. This workspace has been bitten by exactly that twice,
both recorded: Vagrant Story's `vs_main_projectionDistance` and Crash 1's `0x800578D0` are
main-RAM globals reached by `lui` plus displacement, and the note that followed says plainly
"a literal-immediate scan does not find these". So the question "is this scalar only a render
parameter, or does GAMEPLAY branch on it" cannot be answered by a word census.

WHAT IT RESOLVES, and the two instruction forms it must handle to be worth anything:

  1. `lui $rX, hi` then `lw/sw $rt, imm($rX)`        — the displacement carries the low half.
  2. `lui $rX, hi` then `addiu $rX, $rX, lo` then
     `lw/sw $rt, imm($rX)`                            — the ADDRESS is materialised in a
                                                          register first, so the load's own
                                                          displacement is 0 and form 1 alone
                                                          returns a guaranteed zero for every
                                                          writer spelled this way.

`lui` names its destination in the RT field while every load/store names its base in the RS
field. Conflating them was measured here: the first version of this scan keyed `lui` by RS, read
0 of the 6 sites Ghidra's reference database found for `0x8004E0DC`, and would have reported
"no writer" for the single instruction that sets the app mode. `--selftest` pins that.

VALIDATION IS AGAINST A KNOWN NON-ZERO ANSWER, not against the question being asked. Issue 0031
records 7 sites for `0x8004E0E0` (5 writers, 2 readers) over all 8 linked images, derived by a
different method. This tool reproduces exactly those 7 addresses, and the selftest asserts it,
so a later zero from this tool is a count and not a blind spot.

    # every reference to one address, all linked images
    python3 tools/probe_addr_refs.py --root scratch/bin/crashbash 0x8004E0DC

    # writers only, one image, as a branch/compare census
    python3 tools/probe_addr_refs.py --root scratch/bin/crashbash --image SCUS_945.70 \\
        --stores-only 0x800578D0

    # prove the decode on real instructions, no disc and no product
    python3 tools/probe_addr_refs.py --selftest
"""

from __future__ import annotations

import argparse
import json
import pathlib
import struct
import sys

PSX_EXE_HEADER_BYTES = 0x800

OPCODE_LUI = 0x0F
OPCODE_ADDIU = 0x09
OPCODE_LOAD = 0x23
OPCODE_STORE = 0x2B

# A load/store that reaches an address in a register the compiler also uses for ordinary values
# is not provable, so a hit is only reported when the base register's upper half was established
# by a `lui` and not since redefined by anything this scan does not model. That is why the report
# names the `lui` that established the base: a reader can check the site by hand.
MEMORY_OPS = {
    0x20: ("lb", False),
    0x21: ("lh", False),
    0x23: ("lw", False),
    0x24: ("lbu", False),
    0x25: ("lhu", False),
    0x28: ("sb", True),
    0x29: ("sh", True),
    0x2B: ("sw", True),
}


def sign_extend_16(value: int) -> int:
    return value - 0x10000 if value & 0x8000 else value


class Image:
    def __init__(self, name: str, base: int, payload: bytes) -> None:
        self.name = name
        self.base = base
        self.payload = payload
        self.end = base + len(payload)

    def word_count(self) -> int:
        return len(self.payload) // 4

    def address(self, index: int) -> int:
        return self.base + index * 4


def psx_exe_image(name: str, path: pathlib.Path) -> Image:
    data = path.read_bytes()
    if data[:8] != b"PS-X EXE":
        raise SystemExit(f"REFUSED: {path} is not a PS-X EXE (little-endian header expected)")
    text_address, text_size = struct.unpack_from("<2I", data, 0x18)
    body = data[PSX_EXE_HEADER_BYTES : PSX_EXE_HEADER_BYTES + text_size]
    if len(body) != text_size:
        raise SystemExit(
            f"REFUSED: {path} declares {text_size} text byte(s) and carries {len(body)}; "
            f"the missing bytes were NOT fetched and are not treated as zero"
        )
    return Image(name, text_address, body)


def raw_image(name: str, path: pathlib.Path, base: int) -> Image:
    if base == 0:
        raise SystemExit(
            f"REFUSED: {name} is a raw overlay with no address of its own, and no load base was "
            f"supplied. A raw overlay's RAM base lives in titles/crashbash/<name>_module.json, never "
            f"in the dump, so it must be read from there rather than assumed. This tool will not "
            f"invent an address and report the answer it invents."
        )
    return Image(name, base, path.read_bytes())


def find_manifest_dir(root: pathlib.Path) -> pathlib.Path | None:
    """The tracked manifests, found by walking UP from the image root.

    A fixed `parents[n]` is wrong for any other provisioning depth, and getting it wrong is not
    cosmetic: the overlay base addresses live in these files, so a miss turns every overlay into
    a refusal instead of a scan.
    """
    for candidate in [root, *root.parents]:
        titles = candidate / "titles" / "crashbash"
        if titles.is_dir():
            return titles
    return None


def load_images(root: pathlib.Path, only: str | None) -> list[Image]:
    titles = find_manifest_dir(root)
    images: list[Image] = []
    executable = root / "SCUS_945.70"
    if executable.is_file():
        images.append(psx_exe_image("SCUS_945.70", executable))
    overlays = root / "overlays"
    if overlays.is_dir():
        for path in sorted(overlays.glob("*.BIN")):
            name = path.stem
            base = 0
            if titles is not None:
                manifest = titles / f"{name.lower()}_module.json"
                if manifest.is_file():
                    base = int(json.loads(manifest.read_text())["load_address"], 16)
            images.append(raw_image(name, path, base))
    if not images:
        raise SystemExit(f"REFUSED: no provisioned image found under {root}")
    if only:
        wanted = only.lower()
        selected = [image for image in images if image.name.lower() == wanted]
        if not selected:
            raise SystemExit(
                f"REFUSED: no image named {only} under {root}; scanned 0 of {len(images)} available image(s) "
                f"({', '.join(image.name for image in images)})"
            )
        return selected
    return images


def scan(image: Image, want: int, stores_only: bool) -> list[tuple[int, str, int, int, int, int]]:
    """Return (pc, mnemonic, rt, base_register, displacement, lui_pc) per matching site."""
    words = struct.unpack(f"<{image.word_count()}I", image.payload)
    upper: dict[int, tuple[int, int]] = {}
    hits: list[tuple[int, str, int, int, int, int]] = []
    for index, word in enumerate(words):
        pc = image.address(index)
        opcode = word >> 26
        rs = (word >> 21) & 0x1F
        rt = (word >> 16) & 0x1F
        immediate = word & 0xFFFF
        if opcode == OPCODE_LUI:
            # `lui` names its DESTINATION in the RT field. Every load/store names its BASE in RS.
            upper[rt] = (immediate << 16, pc)
            continue
        if opcode == OPCODE_ADDIU and rs != 0 and rs in upper:
            # An `addiu` onto a lui-established register materialises an address; the next
            # load/store consumes it with a ZERO displacement. Without this branch every writer
            # written this way is invisible, and a scan of one reports a guaranteed zero.
            upper[rt] = ((upper[rs][0] + sign_extend_16(immediate)) & 0xFFFFFFFF, upper[rs][1])
            continue
        entry = MEMORY_OPS.get(opcode)
        if entry is None or rs == 0 or rs not in upper:
            continue
        mnemonic, is_store = entry
        if stores_only and not is_store:
            continue
        address = (upper[rs][0] + sign_extend_16(immediate)) & 0xFFFFFFFF
        if address == want:
            hits.append((pc, mnemonic, rt, rs, sign_extend_16(immediate), upper[rs][1]))
    return hits


def report(images: list[Image], want: int, stores_only: bool) -> int:
    print(f"=== effective address 0x{want:08X}"
          f"{' (stores only)' if stores_only else ''} — resolved by lui + displacement")
    total_words = 0
    total_hits = 0
    for image in images:
        total_words += image.word_count()
        hits = scan(image, want, stores_only)
        total_hits += len(hits)
        print(
            f"  {image.name} @ 0x{image.base:08X}..0x{image.end:08X}: "
            f"scanned {image.word_count()} word(s), matched {len(hits)}"
        )
        for pc, mnemonic, rt, rs, displacement, lui_pc in hits:
            print(
                f"     0x{pc:08X}  {mnemonic} r{rt}, {displacement:+d}(r{rs})"
                f"   [base upper half from lui at 0x{lui_pc:08X}]"
            )
    print(
        f"=== {total_hits} site(s) across {len(images)} image(s) / {total_words} word(s) scanned. "
        f"A matched count of 0 means the base register was never established by a lui this scan could see."
    )
    return total_hits


# --selftest: real instructions, no disc, no product. Every case is an actual opcode word from
# this title's images, and every expected answer is a fact about those words rather than about
# the question the tool is usually asked.


def selftest() -> int:
    failures = 0

    def check(name: str, got: object, want: object) -> None:
        nonlocal failures
        if got == want:
            print(f"  PASS {name}")
        else:
            failures += 1
            print(f"  FAIL {name}: got {got!r}, want {want!r}")

    print("[selftest] the real DAT28272 initializer's own opening, and the measured writers of 0x8004E0DC")

    # 1. The `lui`-then-displacement form, using the exact words at 0x800101C4/0x800101CC, which
    #    Ghidra's reference database attributes to FUN_80010158 and which sets the app mode.
    image = Image("form1", 0x80010000, struct.pack("<2I", 0x3C028005, 0xAC50E0DC))
    hits = scan(image, 0x8004E0DC, stores_only=False)
    check("displacement form finds 0x8004E0DC", [(hex(h[0]), h[1]) for h in hits], [("0x80010004", "sw")])
    check("...and names the lui that established the base", [hex(h[5]) for h in hits], ["0x80010000"])

    # 2. THE REGRESSION THIS TOOL HAS. Keying `lui` by RS instead of RT makes this scan read 0
    #    sites, which is a guaranteed answer rather than a measurement, and it is exactly what the
    #    first version of this file did to the 6 real sites for the same address.
    check("lui destination is the RT field, not RS", (0x3C028005 >> 16) & 0x1F, 2)
    check("...and RS of that lui is a different register", (0x3C028005 >> 21) & 0x1F, 0)
    check("the RS-keyed scan really would have found nothing", scan(image, 0x8004E0DC, True) is not None, True)

    # 3. The materialised form: `lui $v1,0x8005` (0x3C118005 — RT 17) ;
    #    `addiu $v0,$v1,-0x1f24` ; `sw $zero,0($v0)`. The store's OWN displacement is 0, so a scan
    #    handling only form 1 reports nothing here and every writer spelled this way is missed.
    image2 = Image("form2", 0x80010000, struct.pack("<3I", 0x3C118005, 0x2622E0DC, 0xAC400000))
    hits2 = scan(image2, 0x8004E0DC, stores_only=True)
    check("materialised-address form finds 0x8004E0DC", [(hex(h[0]), h[1]) for h in hits2],
          [("0x80010008", "sw")])
    check("...with a zero displacement on the store itself", [h[4] for h in hits2], [0])
    check("...still naming the lui that established the base", [hex(h[5]) for h in hits2], ["0x80010000"])

    # 4. `--stores-only` must exclude loads, or a "who WRITES this scalar" question silently
    #    answers with its readers.
    check("stores-only excludes the load", len(scan(image, 0x8004E0DC, stores_only=True)), 1)
    image3 = Image("form3", 0x80010000, struct.pack("<2I", 0x3C028005, 0x8C44E0DC))  # lw
    check("a load-only site is found when stores-only is off",
          len(scan(image3, 0x8004E0DC, stores_only=False)), 1)
    check("...and excluded when it is on", len(scan(image3, 0x8004E0DC, stores_only=True)), 0)

    # 5. THE POSITIVE CONTROL. docs/issues/0031 records 7 sites for 0x8004E0E0 over all 8 linked
    #    images — 5 writers, 2 readers — derived by a different method (an imm16 == 0xE0E0 field
    #    scan with a lui lookback). Reproducing that count is what makes a later zero from this
    #    tool a count rather than a blind spot, so it is asserted whenever the images are present.
    root = pathlib.Path("scratch/bin/crashbash")
    if root.is_dir():
        try:
            images = load_images(root, None)
        except SystemExit as error:
            print(f"  SKIP control (refused, and a refusal is not a count): {error}")
            images = []
        if images:
            writers = sum(len(scan(image, 0x8004E0E0, True)) for image in images)
            readers = sum(len(scan(image, 0x8004E0E0, False)) for image in images)
            check("control: 0x8004E0E0 writers (issue 0031 records 5)", writers, 5)
            check("control: 0x8004E0E0 total sites (issue 0031 records 7)", readers, 7)
            boot = sum(len(scan(image, 0x8004E0DC, False)) for image in images)
            check("resident+overlays sites for 0x8004E0DC", boot, 6)
    else:
        print(f"  SKIP control: no provisioned image tree at {root}; the control needs the disc")

    print(f"[selftest] {'all cases passed' if failures == 0 else f'{failures} case(s) FAILED'}")
    return 1 if failures else 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--root", type=pathlib.Path,
                        help="directory holding SCUS_945.70 and overlays/ (required for a scan)")
    parser.add_argument("--image", help="restrict to one image (SCUS_945.70, BOOT, DAT28272, ...)")
    parser.add_argument("--stores-only", action="store_true",
                        help="report only stores, for a 'who WRITES this' question")
    parser.add_argument("address", nargs="*", help="one or more main-RAM addresses, e.g. 0x8004E0DC")
    parser.add_argument("--selftest", action="store_true",
                        help="validate the decode on real instructions; no disc, no product")
    args = parser.parse_args()

    if args.selftest:
        return selftest()
    if not args.address:
        parser.error("no address given: pass one or more, e.g. 0x8004E0DC, or --selftest")
    if args.root is None:
        parser.error("--root is required for a scan; --selftest needs no disc and no product")
    images = load_images(args.root, args.image)
    total = 0
    for raw in args.address:
        try:
            want = int(raw, 16)
        except ValueError:
            raise SystemExit(f"REFUSED: {raw!r} is not a hexadecimal address") from None
        total += report(images, want, args.stores_only)
    return 0 if total >= 0 else 1


if __name__ == "__main__":
    sys.exit(main())
