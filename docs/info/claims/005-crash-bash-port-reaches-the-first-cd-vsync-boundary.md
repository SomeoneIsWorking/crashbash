---
id: C005
kind: claim
status: holds
created: 2026-08-21
tags: boot,harness,irq
depends: tools/verify_boot.py#judge, game/recomp_seeds.json, game/core/main.cpp, psxport.pin
reconfirmed: 2026-08-21
verified_at: 2026-08-21 14:13:10
falsified_on: 2026-08-21
---

## Claim

The Crash Bash port reaches guest main and its ordered CD IRQ service, finishes the first file load,
then stops at the measured unloaded overlay entry 0x80092BDC.

## Evidence

The Clang-built port pinned to psxport `3418a79b` reports CRT0 audit 10 agree / 0 disagree / 0
unresolved, guest main 0x8002718C, the saved-context/master-dispatcher/CD-service sequence
0x80031AE8 -> 0x80031B58 -> 0x8003F5F0 -> 0x8003E14C, `load file start`, and `done loading`.
It then fails fast at unloaded entry 0x80092BDC (caller 0x80010478), with no earlier unexpected miss,
CD timeout, or cant-find result. The shipping verifier demonstrates the positive and four controlled
negative answers.

## What would falsify it

A verified-image run omits a required ordered reachability marker, reports a CRT0 disagreement,
fails to print `done loading`, reaches a different miss before 0x80092BDC, or a forced-negative
mutation passes.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b: crashbash_boot_boundary_selftest 5/5 required ordered IRQ service, done loading, and exact next miss 0x80092BDC; the full Clang CTest suite passed 7/7.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b Clang build: live boot verifier passed 5/5 with ordered IRQ service,
`done loading`, and exact next miss 0x80092BDC; full CTest passed 7/7. The zero-argument
`./run.sh` path independently built the same current target, loaded the file once, and stopped at
that same declared incomplete boundary.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b Clang build: live boot verifier passed 5/5, full CTest passed 7/7, direct and zero-argument launcher paths each loaded once and stopped at exact next miss 0x80092BDC.
