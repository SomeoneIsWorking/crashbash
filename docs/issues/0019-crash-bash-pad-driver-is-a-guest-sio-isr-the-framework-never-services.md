---
id: 19
title: Crash Bash's pad driver is a guest SIO ISR the framework never services — no button ever reaches guest RAM
status: open
symptom: START presses (dbg server) reach the host pad layer but never guest RAM (pressed-vs-idle 2MB diff at a fixed frame: zero button-state sites). The attract demo therefore never exits on input and the app mode never leaves the BOOT loop (0x80078C90 / 0x8004E0B8) even at 40000 frames. The guest makes zero BIOS pad calls and zero SIO MMIO accesses at runtime — because the driver that would is never serviced.
state_items: S002
tags: pad,input,sio,irq-delivery,attract,menu,re-frontier,d32
created: 2026-08-29
updated: 2026-08-29
---

## Root cause (measured, see docs/findings/crashbash-pad-sio.md)

The title builds its own pad engine: it patches the kernel C0/B0 tables returned by
B0:0x56/0x57 with its own code blobs, then registers `SysEnqIntRP(class=2, elem=0x8006D984)`
whose verifier `FUN_8003b224` / handler `FUN_8003b1BC` drive SIO0 through the runtime pointer
`DAT_8006d99c = 0x1F801040` (CTRL bit 1, BAUD=0x88, a 0xF0-stride slot table at 0x8007765C).
The framework models no SIO0 hardware and asserts no SIO interrupt, so the verifier never
claims, the handler never runs, and no button state is ever written anywhere in RAM.

## The fix

Model SIO0 registers (0x1F801040–0x1F80104F) in `Core::io_read/io_write` and deliver the SIO
interrupt per completed controller transfer (per-vblank cadence), so the guest's own ISR runs
the handshake. The guest-side contract is fully measured in the finding; the host side is a
small pad state machine plus an I_STAT bit-7 source through the existing `Hle::irqEnq` chain
(already registered, "chain now 1"). Rejected shortcut: writing masks into `0x8007787C` /
`padSlot0Buf` — that buffer is written by nothing and read through no established path; a mask
with no ISR is a fabricated input path, not a port.

## Verification gate

With the model in place, pressing START through the dbg server must (a) make the button mask
appear in guest RAM at the driver's slot table, and (b) move the app mode off 0x80078C90 —
the menu transition that every 40000-frame run so far has failed to reach.
