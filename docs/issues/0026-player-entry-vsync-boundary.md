---
id: 26
title: Direct Crash Bash player entry stops at application initialization VSync boundary
status: resolved
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

## Root cause and resolution

The direct `PlatformHlePlan` declared only VSync. The former title `GameConfig` also declared
`0x8003EBF8` (stock CdCommand), `0x8003E6B0` (stock CdSync), and `0x80034C6C` (stock CdSearchFile).
Those entries were bound by `Cd::overridesInit` only when `core.cfg` existed, so the direct player
left the synchronous native CD services unbound and entered the retail controller/VSync timeout body.

The direct plan now carries typed addresses for those three existing framework owners. Crash Bash
supplies its measured addresses; the ISO search implementation is the same owner used by the legacy
registration path. Focused Clang tests prove all three direct bindings, preserve the three legacy
bindings and stock command/sync behavior, and leave an unrelated address unbound.

A bounded direct player run against the authenticated USA executable and CHD installed four
hardware services, completed `load file start` / `done loading`, and entered the native frame loop
with no guest VSync trap. It then failed fast at BOOT entry `0x80092BDC` because the loaded image has
not been published to the authenticated image catalog; issue 0027 owns that next boundary. The run
does not establish gameplay or presentation conformance.
