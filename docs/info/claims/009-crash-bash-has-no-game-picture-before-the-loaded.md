---
id: C009
kind: claim
status: holds
created: 2026-08-22
tags: boot,graphics,overlay
depends: CMakeLists.txt, game/core/recomp_register.cpp, game/recomp_seeds.json, tools/verify_boot.py#judge
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
