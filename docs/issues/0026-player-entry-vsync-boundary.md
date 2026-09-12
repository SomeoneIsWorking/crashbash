---
id: 26
title: Direct Crash Bash player entry stops at application initialization VSync boundary
status: investigating
symptom: The linked player executable authenticates the resident image and enters boot, then aborts when 0x80012E90 returns a FrameBoundary instead of GuestReturn
state_items: S002,S003,S015
tags: boot,dynarec,player-entry,vsync,cd
created: 2026-09-12
updated: 2026-09-12
---

## Evidence

`game/core/player_entry.cpp` now creates the heap-owned `Game`, loads the user-supplied
`SCUS_945.70` through the authenticated resident loader, publishes the image-qualified native
owners, binds the per-Core hardware owners, and enters `native_boot_run`. The Clang/Ninja target
links and the focused Crash Bash tests pass.

A real headless run reaches heap setup, the finite boot prefix, and the real CHD open. It then
reports:

`0x80012E90 -> 0x800279A4 -> 0x80034AFC -> 0x80034B8C -> 0x8003F29C -> 0x8003EBF8 -> 0x8003E6B0 -> 0x800320EC`

The strict VSync service requests `FrameBoundary` after 128 cycles. `measuredGuestCall` correctly
refuses to treat that bounded exit as a completed startup call, so the process aborts. This is the
first direct-player runtime falsifier; the VSync trap must remain strict.

## Next owner

Make the measured application/CD initialization operation resumable across a frame boundary, or
replace it with a complete title-owned synchronous operation preserving the authenticated disc,
guest state, and instruction accounting contracts. Do not ignore `FrameBoundary`, fabricate a VSync
value, relax the watchdog, or reroute this startup through an interpreter.
