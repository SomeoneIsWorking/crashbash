# Crash Bash pad input — guest SIO driver and the three missing framework hardware owners

Measured 2026-08-29, from the retail exe + `scratch/raw/miss_ram.bin` Ghidra project
(`scratch/ghidra/cbre`) + runtime traces (`scratch/logs/bios-census2.log`, `io-census2.log`).
The original negative capture proved no host input reached guest RAM. A fresh 2026-08-30 A/B
after the framework implementation proves the complete guest input path now works. Resolved issue
0020 later proved the active retail menu deliberately ignores START; resolved issue 0021 proves its
Cross accept transition.

## The symptom chain

1. The port's host pad layer (`Pad::press start`) works, but `GameConfig::padSlot0Buf` is 0 for
   this title, so the framework's per-frame buffer fill has nowhere to write. That is fine —
   see below, this game does not use that mechanism at all.
2. The guest makes **zero** BIOS pad calls in 9000 frames (bios census: no A0:0x12–0x16, no
   B0:0x12/0x13) and **zero** runtime SIO MMIO accesses (io census: no reads/writes of
   0x1F801040–0x1F80104F). The exe also contains no SIO register immediates (lui/addiu scan) —
   the driver reaches SIO through a runtime pointer instead.
3. Before the framework fix, the button mask never entered guest RAM: the pressed-vs-idle 2 MiB
   RAM diff at a fixed frame differed at exactly zero button-state sites.

## The driver, measured

- `B0:0x56`(GetC0Table)/`B0:0x57`(GetB0Table) are called from the pad-init region
  (`0x800486DC` kMemoryCardStartup, `0x8004BC64`–`0x8004BE58`), and the exe **patches the
  returned kernel tables in place**: C0-table `+0x18`, `+0x70` (2 words from `0x8005BEFC`),
  `+0x28+Σ` (5 words from `0x8005BD00`), `+0x9C8` (10 words from `0x8005BD28`), and copies the
  blob `0x8005BC90..0x8005BD00` over the table body. The installer is `jal 0x80048DFC`, which
  also stores the table pointer at `0x1FDFFC` — the SIO ISR tail (`0x8004BCE4: lw r2,0x1FDFFC;
  jr r2`) returns through it.
- The guest registers an InterruptElement: `C0:0x02 SysEnqIntRP(class=2, elem=0x8006D984)`
  from `0x80031FE4`. The element is `+0=0`, `+4=0x8003B224` (handler),
  `+8=0x8003B1BC` (verifier). The framework already chains this (`Hle::irqEnq`,
  "chain now 1").
- The verifier tests VBlank `I_MASK/I_STAT` bit 0. The driver's state globals live at
  `0x8006D924`–`0x8006D99C`; `DAT_8006d99c = 0x1F801040` (SIO0 DATA base). The handler
  `FUN_8003b224` reads SIO_CTRL (`base+0xA = 0x1F80104A`) bit 1,
  manages a 0xF0-stride pad/slot table at `0x8007765C`, writes SIO_BAUD
  (`base+0xE = 0x1F80104E`) = `0x88`, and drives the transfer through `FUN_8003b3b4` /
  `FUN_8003b6e8` / indirect `0x80040428`. The wait loop at `0x8004BCD0` (`lw 0x1044($v1)`,
  test bit 0x80, timeout through `0x1FDFFC`) is the polling twin of the same driver.

## The three framework omissions

1. Completed physical fields did not raise VBlank `I_STAT` bit 0, so the element's verifier had
   no real display interrupt to claim.
2. SIO0 `0x1F801040..04F` was unmapped, so even a handler entry had no controller response or
   transfer/`/ACK` deadlines. Per-byte acknowledgement is SIO `I_STAT` bit 7, distinct from the
   VBlank bit that starts the handler.
3. Root counter 2 `0x1F801120..128` read as zero. The driver latches that counter and spins on
   a delta for both pre-ACK delay and timeout; a constant zero makes both loops non-progressing.

`0x8007787C` (an InitPAD-shaped 0x22-byte buffer whose registration shows up as leftover call
arguments) is written by nothing during the run and is not this title's input path.

## The shared framework implementation

`io_peripherals.{h,cpp}` owns the interrupt/SIO/root-counter address decode.
`Timing` raises VBlank for completed whole fields and owns timer-2 stopwatch semantics.
`Sio0` owns one digital pad on port 1: the bytes shifted back are `FF`, `41`, `5A`,
buttons-low, buttons-high for transmit bytes `01`, `42`, `00`, `00`, `00`; the final byte is
not acknowledged. Transfer time derives from JOY_MODE/JOY_BAUD. The 64-clock ACK delay and
32-clock active pulse are the values used by the vendored Beetle `frontio.c` oracle, and land
inside the independently measured Crash Bash pre-clear/timeout window.

The model raises SIO `I_STAT` bit 7 only at the actual enabled ACK deadline. It does not inject
pad masks, fabricate timer interrupts, answer the absent second port, or couple SIO ACK to
VBlank cadence.

## Fresh product A/B

Two finite, headless, no-audio, no-pacing 210-frame runs against exact recorded psxport
`a390ceed` dumped RAM at frame 200. The Crash Bash Clang suite passes 23/23, including the
framework-pin and C++ policy gates. Idle used active-low host mask `FFFF`; START used `FFF7`.

| Boundary | Address | Idle | START |
|---|---:|---:|---:|
| guest driver packet | `0x80077FBC` | `41 5A FF FF` | `41 5A F7 FF` |
| parsed P1 halfword | `0x80063A92` | `FFFF` | `FFF7` |
| game-facing active-high P1 | `0x8005133C` | `0` | `8` |
| game-facing P2 | `0x80051394` | `0` | `0` |

The complete RAM SHA-256 values are
`26ba65c60ac8cffe5e17682351ef456ecf27fd3dcc6c5a94e3d5385de2373a88` (idle) and
`266f3be554e29497be4e489f5bf7dc5858b5e2a55802bdabf1eeb7126617b893` (START).
This closes input delivery only. Resolved issue 0020 proves this active menu does not accept START;
resolved issue 0021 proves the idle/START/Cross transition differential.

Ghidra: project `scratch/ghidra/cbre` (imported 2MB `scratch/raw/miss_ram.bin` @ 0x80000000);
decomp source for this note: `scratch/decomp/8003b224.c`, `scratch/decomp/8003fd00.c`,
`scratch/decomp/800401e4.c`. Script `scratch/ghidra_scripts/DecompDump.py`
(`CB_DECOMP_TARGETS=0x...` env).
