---
id: C006
kind: claim
status: holds
created: 2026-08-21
tags: boot,harness,irq
depends: tools/verify_oracle_irq.py#compare, tools/verify_boot.py#judge, game/recomp_seeds.json, psxport.pin
reconfirmed: 2026-08-21
verified_at: 2026-08-21 11:43:16
---

## Claim

Crash Bash custom interrupt exit agrees with the true Beetle oracle and reaches the CD IRQ service

## Evidence

Independent Beetle interpreter on the actual USA CHD showed steady v0=1, sp=0x80068B14 transitions 0x80031AE8 -> 0x80031B58 with dispatcher ra=0x80031AF8. The Clang-built port on pinned psxport 692b9b20 showed the same prefix then 0x8003F5F0 -> 0x8003E14C, with no recomp miss or CD timeout. Both verifiers passed all forced negatives.

## What would falsify it

A repeat true-oracle or port trace changes the saved-context ordering/state, omits the CD callback/drain, reports a recomp miss/CD timeout, or any forced-negative trace is accepted.

## Re-confirmed 2026-08-21

Post-landing true Beetle/port interrupt comparison remained green and its forced-answer comparator selftest passed 4/4; the port reaches the ordered CD service path.
