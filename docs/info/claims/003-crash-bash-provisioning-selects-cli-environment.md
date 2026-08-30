---
id: C003
kind: claim
status: holds
created: 2026-08-21
tags: boot,provisioning
depends: tools/loaded_module.py#verify_source, tools/provision.py#provision, titles/crashbash/dat28136_module.json
reconfirmed: 2026-08-30
verified_at: 2026-08-30 04:46:17
---

## Claim

Crash Bash provisioning selects CLI > environment > .env > one drop-in, refuses an invalid authoritative source or ambiguity, and publishes the executable plus every registered code module only after the complete measured input set verifies

## Evidence

tests/test_provision.py exercises positive, mismatch, refusal, precedence, ambiguity, shared-source,
bounded-offset, entryless-module, and atomic-publication behavior through tools/provision.py. The real
USA CHD passes SYSTEM.CNF 1/1, executable identity/header 11/11, and 32/32 module facts across BOOT
plus MENU, DAT28272, DAT28241, and DAT28136.

## What would falsify it

A source can silently fall through, any input is published before the complete registered set verifies,
a tracked executable or module field is not checked, or the real selected disc no longer provisions
11/11 executable facts and the denominator implied by `loaded_module.MODULES`.

## Re-confirmed 2026-08-21

Post-landing recheck: tests/test_provision.py passed 8/8 through the tracked shipping provisioner; real USA media had already passed SYSTEM.CNF 1/1 and executable identity/header 11/11.

## Re-confirmed 2026-08-21

Post-landing crashbash_provision_selftest passed its resolution, refusal, identity, and atomic-preservation cases.

## Re-confirmed 2026-08-22

Post-commit provisioning selftest passed all 12 executable/module identity, refusal, unknown-field, and publication controls; the preceding real-media gate passed executable 11/11 and modules 14/14.

## Re-confirmed 2026-08-22

Post-commit provisioning selftest passed all 12 executable/module identity, refusal, unknown-field, and publication controls; the preceding real-media gate passed executable 11/11 and modules 14/14.

## Re-confirmed 2026-08-30

Real USA media passed SYSTEM.CNF 1/1, executable 11/11, and loaded modules 32/32; tests/test_provision.py passed in both the 24/24 Clang CTest gate and the 26-test Python suite.

## Re-confirmed 2026-08-30

Final tree: real USA provisioning passed 1/1 SYSTEM.CNF, 11/11 executable, and 32/32 module facts; final Clang CTest passed 24/24 and Python suite passed 26/26.
