---
id: 20
title: The active Crash Bash menu was incorrectly expected to accept START
status: resolved
symptom: Fresh headless START A/B proved the guest SIO packet, parsed P1 word, and game-facing active-high state all changed correctly, but no menu transition occurred because the active retail callback does not bind START.
state_items: S002
tags: pad,input,flow,attract,menu,re-frontier
created: 2026-08-30
updated: 2026-08-30
---

## Root cause

The investigation assumed START should accept the active menu. Retail code disproves that premise.
`0x8005133C` is held/current pad state, while the rising edge read by menu code is P1 object `+0x48`
at `0x80051380`. The existing trace already shows START edge `8` reaches `FUN_8007F314`.

The current state index at `0x8005A648` is `0x28`; its table type is `0x0101`. `FUN_8007F314`
compares that type at `0x8007F3C0` and takes the branch at `0x8007F3C4`, returning before the generic
START scan at `0x80080860-0x8008086C`. This is the retail control flow, not a port divergence.

The verified active process manager points to table `0x800B8E28`, whose update callback is
`FUN_800B3CA8`. That callback reads rising edge `0x80051380`, handles Up `0x40`, Down `0x10`, and
accepts Cross `0x4000` at `0x800B3D88-0x800B3D8C`. With selection zero, Cross calls
`FUN_800B5360` and schedules table `0x800B8E50` through pending manager slot `0x8009F8A8`. Every
MENU-module xref to the edge word was checked; none masks START bit `8`.

## What was tried / dead ends

- Feeding `padSlot0Buf` or `0x8007787C` is rejected: Crash Bash reads its own guest SIO
  pipeline, which issue 0019 now proves operational.
- Extending the run to 9000/40000 frames does not make held START transition on its own.
- Treating unchanged BOOT vtable/process-object pointers as a failed pad gate conflates two
  boundaries; the pad state changes correctly while those pointers remain stable.
- Treating held/current word `0x8005133C` as the decision word skipped the rising-edge owner at
  `0x80051380`.

## Resolution

The START failure is not a product defect; it matches the active retail type-`0x101` menu. Resolved
issue 0021's automated shipping differential proves idle and START preserve pending slot zero, while
Cross traverses the measured accept path and schedules `0x800B8E50` from the same checkpoint.
