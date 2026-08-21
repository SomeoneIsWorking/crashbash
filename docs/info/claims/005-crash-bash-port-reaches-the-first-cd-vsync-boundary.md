---
id: C005
kind: claim
status: holds
created: 2026-08-21
tags: boot,harness,irq
depends: tools/verify_boot.py#judge,game/recomp_seeds.json,game/core/main.cpp
reconfirmed: 2026-08-21
verified_at: 2026-08-21
---

## Claim

The Crash Bash port reaches guest main and its measured IRQ callback without a recomp miss, then stops at the unimplemented CD/VSync hardware boundary.

## Evidence

The real 96-line boot trace reported CRT0 audit 10 agree / 0 disagree / 0 unresolved, correct InitHeap arguments, guest main 0x8002718C reached, and IRQ callback 0x8003B1BC reached. No recomp miss or segmentation fault occurred. The remaining output repeatedly reported CD timeout, an unclaimed IRQ source, and watchdog stuck. The shipping verifier passed this positive and rejected both a trace with guest-main reachability removed and one with a forced recomp miss.

## What would falsify it

A real verified-image run omits either reachability trace, reports a CRT0 disagreement/unresolved field or recomp miss, reaches a different stop before the CD/VSync diagnostics, or the forced-negative mutations cease to fail.
