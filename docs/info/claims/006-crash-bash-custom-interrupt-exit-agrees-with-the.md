---
id: C006
kind: claim
status: holds
created: 2026-08-21
tags: boot,harness,irq
depends: tools/verify_oracle_irq.py#compare, tools/verify_boot.py#judge, game/recomp_seeds.json, psxport.pin
reconfirmed: 2026-08-22
verified_at: 2026-08-22 14:13:50
---

## Claim

Crash Bash custom interrupt exit agrees with the true Beetle oracle and reaches the CD IRQ service

## Evidence

Independent Beetle interpreter on the actual USA CHD showed steady v0=1, sp=0x80068B14 transitions
0x80031AE8 -> 0x80031B58 with dispatcher ra=0x80031AF8. The Clang-built port showed the same
prefix then 0x8003F5F0 -> 0x8003E14C, with no unexpected miss or CD timeout before that boundary.
Both verifiers passed all forced negatives. The later expected overlay-discovery miss at 0x80092BDC
is outside this claim.

## What would falsify it

A repeat true-oracle or port trace changes the saved-context ordering/state, omits the CD
callback/drain, reports an unexpected miss or CD timeout before that service boundary, or any
forced-negative trace is accepted.

## Re-confirmed 2026-08-21

Post-landing true Beetle/port interrupt comparison remained green and its forced-answer comparator selftest passed 4/4; the port reaches the ordered CD service path.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b preserves the true-oracle 0x80031AE8 -> 0x80031B58 prefix and reaches 0x8003F5F0 -> 0x8003E14C before any unexpected miss; IRQ comparator forced answers remain green.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b preserved the true-oracle saved-context/master-dispatcher prefix and the CD IRQ callback/drain; oracle comparator forced answers passed 4/4.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b preserves the true-oracle saved-context/master-dispatcher prefix and CD IRQ callback/drain; oracle comparator forced answers passed 4/4.

## Re-confirmed 2026-08-21

Post-landing IRQ comparator remained 4/4 and live execution reached the ordered CD drain without nested ISR frames; the separate completion-coalescing residual remains explicitly refused.

## Re-confirmed 2026-08-22

Pinned psxport 7f5d3f13 live execution retained the measured 0x80031AE8 -> 0x80031B58 -> 0x8003F5F0 -> 0x8003E14C service order before any unexpected miss; the oracle comparator's controlled-answer selftest passed in full CTest.
