---
id: 19
title: Crash Bash pad input never reaches guest RAM because psxport omitted its interrupt and SIO hardware
status: resolved
symptom: START reaches the host pad layer but a fixed-frame idle/pressed RAM A/B originally differed at zero guest button-state sites; the title makes no BIOS pad calls because it owns a direct SIO0 driver on a registered interrupt element.
state_items: S002
tags: pad,input,sio,irq-delivery,attract,menu,re-frontier,d32
created: 2026-08-29
updated: 2026-08-30
---

## Root cause (measured, see docs/findings/crashbash-pad-sio.md)

The title builds its own pad engine and registers
`SysEnqIntRP(class=2, elem=0x8006D984)`. The element's `+8` verifier at `0x8003B1BC`
tests VBlank in `I_MASK/I_STAT`; its `+4` handler at `0x8003B224` drives SIO0 through
`0x1F801040`, uses SIO `I_STAT` bit 7 for per-byte acknowledgements, and times its
delay/timeout loops with root counter 2.

The shared framework omitted all three hardware owners needed by that real guest path:
completed display fields did not raise VBlank `I_STAT` bit 0, SIO0 MMIO had no controller
or transfer/ACK deadlines, and root counter 2 was unmapped and always read as zero. The
registered verifier therefore did not initiate useful work, and even a forced entry could not
complete the driver's timed byte exchange.

## The fix

The shared psxport runtime now raises VBlank only when a physical display field completes,
models timer-2 value/mode/target stopwatch behavior, and models one digital controller on
SIO0 port 1 with baud-derived transfer deadlines and oracle-derived `/ACK` delay/pulse timing.
The existing `Hle::irqEnq` chain runs the guest verifier and handler; no title-local buffer fill
or synthetic event was added. `io_peripherals.{h,cpp}` owns register decode and
`sio_pad.{h,cpp}` owns the controller protocol.

## Verification gate

At one fixed frame, an idle/START A/B must prove all three shipping boundaries:

- driver packet `0x80077FBC`: `41 5A FF FF` -> `41 5A F7 FF`;
- parsed P1 word `0x80063A92`: `FFFF` -> `FFF7`;
- game-facing active-high P1 state `0x8005133C`: `0` -> `8`.

Port 2 must remain absent rather than duplicating P1. Whether the accepted START changes the
current BOOT/attract flow is separate issue 0020.

### Resolution (2026-08-30)
Root cause was the shared framework omission described above, not guest wiring. The framework fix
landed as psxport `a390ceed`; Crash Bash records that exact pin and its Clang suite passes 23/23.
Fresh finite headless idle/START runs against that pin at frame 200 produce distinct 2 MiB RAM images
`26ba65c60ac8cffe5e17682351ef456ecf27fd3dcc6c5a94e3d5385de2373a88` and
`266f3be554e29497be4e489f5bf7dc5858b5e2a55802bdabf1eeb7126617b893` and satisfy every
packet/parsed/game-facing gate. Port 2 stays absent/zero. The accepted START does not yet leave
BOOT/attract flow; issue 0020 owns that independent boundary.
