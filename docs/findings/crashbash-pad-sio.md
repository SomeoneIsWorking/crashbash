# Crash Bash's pad input path — the SIO driver the framework never services

Measured 2026-08-29, from the retail exe + `scratch/raw/miss_ram.bin` Ghidra project
(`scratch/ghidra/cbre`) + runtime traces (`scratch/logs/bios-census2.log`, `io-census2.log`).
This is why the attract demo never responds to START, not a guess.

## The symptom chain

1. The port's host pad layer (`Pad::press start`) works, but `GameConfig::padSlot0Buf` is 0 for
   this title, so the framework's per-frame buffer fill has nowhere to write. That is fine —
   see below, this game does not use that mechanism at all.
2. The guest makes **zero** BIOS pad calls in 9000 frames (bios census: no A0:0x12–0x16, no
   B0:0x12/0x13) and **zero** runtime SIO MMIO accesses (io census: no reads/writes of
   0x1F801040–0x1F80104F). The exe also contains no SIO register immediates (lui/addiu scan) —
   the driver reaches SIO through a runtime pointer instead.
3. So the button mask never enters guest RAM: the pressed-vs-idle 2MB RAM diff at a fixed frame
   differs at exactly zero button-state sites.

## The driver, measured

- `B0:0x56`(GetC0Table)/`B0:0x57`(GetB0Table) are called from the pad-init region
  (`0x800486DC` kMemoryCardStartup, `0x8004BC64`–`0x8004BE58`), and the exe **patches the
  returned kernel tables in place**: C0-table `+0x18`, `+0x70` (2 words from `0x8005BEFC`),
  `+0x28+Σ` (5 words from `0x8005BD00`), `+0x9C8` (10 words from `0x8005BD28`), and copies the
  blob `0x8005BC90..0x8005BD00` over the table body. The installer is `jal 0x80048DFC`, which
  also stores the table pointer at `0x1FDFFC` — the SIO ISR tail (`0x8004BCE4: lw r2,0x1FDFFC;
  jr r2`) returns through it.
- The guest registers an InterruptElement: `C0:0x02 SysEnqIntRP(class=2, elem=0x8006D984)`
  from `0x80031FE4`. The element is `+0=0 +4=0x8003B224 (verifier) +8=0x8003B1BC (handler)`.
  The framework already chains this (`Hle::irqEnq`, "chain now 1").
- The driver's state globals live at `0x8006D924`–`0x8006D99C`; `DAT_8006d99c = 0x1F801040`
  (SIO0 DATA base). The handler `FUN_8003b224` reads SIO_CTRL (`base+0xA = 0x1F80104A`) bit 1,
  manages a 0xF0-stride pad/slot table at `0x8007765C`, writes SIO_BAUD
  (`base+0xE = 0x1F80104E`) = `0x88`, and drives the transfer through `FUN_8003b3b4` /
  `FUN_8003b6e8` / indirect `0x80040428`. The wait loop at `0x8004BCD0` (`lw 0x1044($v1)`,
  test bit 0x80, timeout through `0x1FDFFC`) is the polling twin of the same driver.

## Why nothing runs

The element is registered but nothing ever raises the event it verifies: the framework models
no SIO0 hardware (io_read/io_write have no 0x1F801040–0x104F handlers), and no I_STAT SIO bit
(class 2) is ever asserted. The verifier `FUN_8003b224` therefore never claims an interrupt and
the handler never clocks a byte. `0x8007787C` (an InitPAD-shaped 0x22-byte buffer whose
registration shows up as leftover call args) is written by nothing all run.

## The faithful fix (next RE/engineering step)

Model SIO0 in `Core::io_read/io_write` (0x1F801040–0x1F80104F: DATA/STATUS/MODE/CTRL/BAUD) and
assert the SIO interrupt per vblank the way real hardware delivers a completed controller
transfer, so the guest's own verifier/handler pair runs and performs the handshake itself.
The guest-side contract is fully measured above; the host side needs a small state machine
(present byte → 0x01 → 0x42 → 0x5A → id 0x41 → buttons lo/hi, active-low) plus an I_STAT bit-7
source. Do NOT shortcut by writing button masks into `0x8007787C` or `padSlot0Buf`: the buffer
is not the mechanism this game reads, and a mask with no ISR is a fabricated input path.

Ghidra: project `scratch/ghidra/cbre` (imported 2MB `scratch/raw/miss_ram.bin` @ 0x80000000);
decomp source for this note: `scratch/decomp/8003b224.c`, `scratch/decomp/8003fd00.c`,
`scratch/decomp/800401e4.c`. Script `scratch/ghidra_scripts/DecompDump.py`
(`CB_DECOMP_TARGETS=0x...` env).
