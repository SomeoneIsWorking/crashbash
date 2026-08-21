---
id: C004
kind: claim
status: holds
created: 2026-08-21
tags: boot,recompiler
depends: tools/recomp_bootstrap.py#check,game/recomp_seeds.json,game/core/game_config.cpp
reconfirmed: 2026-08-21
verified_at: 2026-08-21 11:43:15
---

## Claim

Crash Bash's resident substrate is reproducibly derived from the verified USA executable and measured boot/routing facts, with no configured overlays or guessed explicit seeds.

## Evidence

The shipping bootstrap emitted 339 roots into 907 resident functions over 0x80010000..0x80079000 and verified the required CRT0, dispatch, index, override, and overlay-table interfaces. Its 4/4 selftest accepted the real executable and rejected a changed BSS bound, an explicit seed outside executable text, and a one-byte executable mutation. The sole explicit seed, IRQ callback 0x8003B1BC, came from an observed recomp miss through the guest-RAM callback at 0x8006D98C and was independently decompiled as that callback.

## What would falsify it

The same verified executable emits different routing or interfaces, a shipping GameConfig field disagrees with the executable measurement, an explicit seed lacks runtime and static evidence, generated output is hand-edited or tracked, or an overlay becomes configured without measured load evidence.

## Re-confirmed 2026-08-21

Post-landing crashbash_recomp_bootstrap_selftest passed 4/4 with the real 339-root/907-function substrate and three forced negatives.

## Re-confirmed 2026-08-21

Post-landing recomp_bootstrap selftest passed 6/6; the identity-bound image plus measured indirect verifier and sole main_reentry emit 340 roots into 908 functions.
