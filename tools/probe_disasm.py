#!/usr/bin/env python3
"""probe_disasm.py — diagnosis only.

Disassembles a guest address out of a provisioned PS-X EXE, using the byte order
the whole workspace stores executables in (little-endian MIPS words; every
provisioned image in this workspace, including openbios-fast.bin, stores the
canonical `addiu $sp,$sp,-0x18` as the byte sequence e8 ff bd 27).

  python3 tools/probe_disasm.py <exe> <vaddr> [instruction-count]

The header words are read little-endian, matching tools/extract_exe.py and
external/mmx4/build.py (mipsel-linux-gnu-as), so the PS-X EXE header at 0x18 is
the text address and at 0x1C the text length.
"""
import struct
import sys

from capstone import CS_ARCH_MIPS, CS_MODE_BIG_ENDIAN, CS_MODE_LITTLE_ENDIAN, CS_MODE_MIPS32, Cs

HEADER_BYTES = 0x800
HEADER_FIELDS = ("pc0", "gp0", "t_addr", "t_size")


def main() -> int:
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    path = sys.argv[1]
    address = int(sys.argv[2], 0)
    count = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x40
    data = open(path, "rb").read()
    if data[:8] != b"PS-X EXE":
        print(f"{path} is not a PS-X EXE")
        return 2
    pc0, gp0, t_addr, t_size = struct.unpack_from("<4I", data, 0x10)
    print(f"[probe] {path}: pc0=0x{pc0:08X} gp0=0x{gp0:08X} text=0x{t_addr:08X}+0x{t_size:X} ({len(data)} bytes)")
    if not (t_addr <= address < t_addr + t_size):
        print(f"[probe] 0x{address:08X} is OUTSIDE the loaded text")
        return 1
    offset = HEADER_BYTES + (address - t_addr)
    little = data.count(bytes.fromhex("e8ffbd27"))
    big = data.count(bytes.fromhex("27bdffe8"))
    print(f"[probe] canonical addiu $sp,$sp,-0x18 as little-endian words: {little}, as big-endian: {big}")
    disassembler = Cs(CS_ARCH_MIPS, CS_MODE_MIPS32 + CS_MODE_LITTLE_ENDIAN)
    disassembler.skipdata = True
    print(f"[probe] disassembling 0x{address:08X} (file offset 0x{offset:X})")
    emitted = 0
    for instruction in disassembler.disasm(data[offset : offset + count * 4], address):
        print("  %08X  %-9s %s" % (instruction.address, instruction.mnemonic, instruction.op_str))
        emitted += 1
    # A big-endian reading of the same bytes is printed for the same range: a disassembly that
    # stops after one instruction is the tell that the byte order is wrong.
    other = Cs(CS_ARCH_MIPS, CS_MODE_MIPS32 + CS_MODE_BIG_ENDIAN)
    other.skipdata = True
    decoded = sum(1 for _ in other.disasm(data[offset : offset + count * 4], address))
    print(f"[probe] same bytes read big-endian decode {decoded} instruction(s) vs {emitted} little-endian")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
