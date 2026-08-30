---
id: 21
title: Cross menu acceptance needs an idle/START/Cross differential
status: resolved
symptom: Static retail code says the active type-0x101 MENU callback ignores START and schedules the next table only on Cross, but the shipping port has not yet proven the Cross-positive transition from the same checkpoint.
state_items: S002
tags: pad,input,flow,menu,cross,differential,re-frontier
created: 2026-08-30
updated: 2026-08-30
---

## Root cause

The previous investigation used the wrong action. Static retail analysis predicts that the current
active callback is already behaving correctly and that Cross, not START, is its accept edge. The
shipping port needed a controlled positive answer at the same checkpoint before that finding could be
trusted.

## What was tried / dead ends

- Issue 0020's START expectation is falsified. The active state type `0x0101` exits before the generic
  START scan, and the active MENU callback has no START test.
- Direct-buffer injection remains rejected; issue 0019 proves the title's guest SIO path reaches the
  game edge state.

## Resolution

`game/diagnostics/menu_boundary.cpp` now observes the shipping MENU update and accept owners without
replacing their generated behavior. `tools/verify_menu_accept.py` runs idle, START, and Cross serially,
requires the exact compiled-substrate identity in all three traces, and judges the same measured
checkpoint. Its 7/7 controlled suite proves it rejects a missing identity, idle edge, false START
accept, missing START edge, wrong pending target, and missing Cross accept.

The real Clang product differential passes: idle and START each execute 88 active MENU updates; START
produces three edge-`8` records and zero accepts; Cross produces edge `0x4000`, calls
`FUN_800B5360` exactly once, and changes pending manager `0 -> 0x800B8E50`. Six following updates
preserve current `0x800B8E28` with pending `0x800B8E50`, proving this is a two-phase queued
transition rather than an immediate current-manager swap. The old callback then stops after eight
updates. No START mapping was added.
