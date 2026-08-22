---
id: C009
kind: claim
status: falsified
created: 2026-08-22
tags: boot,graphics,overlay
depends: CMakeLists.txt, game/core/recomp_register.cpp, game/recomp_seeds.json, tools/verify_boot.py#judge
reconfirmed: 2026-08-22
verified_at: 2026-08-22 14:13:51
falsified_on: 2026-08-22
---

## Claim

Crash Bash has no game picture before the first uncompiled loaded entry at 0x80092BDC

## Evidence

The headless product built from the unchanged shipping game sources against recorded psxport pin
3418a79b initialized the native GPU, loaded all 189 sectors of CRASHBSH.DAT, and then failed fast at
0x80092BDC before any game frame. The runtime gate passed with 129 lines and all 7 CTest gates passed
with the verified USA disc. The address is outside every emitted resident/overlay range; CMake registers
no render sources, `game/render/` is absent, and the game hook registers no native override.

## What would falsify it

The loaded executable containing 0x80092BDC is emitted and executes into a game frame, or a verified
native producer is registered and submits a game picture before that boundary.

## Re-confirmed 2026-08-22

Pinned psxport 7f5d3f13 with inherited CrashBashRuntime loaded all 189 DAT sectors and stopped at unloaded entry 0x80092BDC before a game frame; CMake still registers no render source and the runtime owns no native override.

## FALSIFIED 2026-08-22

0x80092BDC is now inside the measured, provisioned, and recompiled BOOT module and execution proceeds through nested MENU, so it is neither the first uncompiled loaded entry nor the current no-picture boundary; C011 supersedes it.

> Anything that cited this claim as proof must be re-checked. Grep the repo for it.
