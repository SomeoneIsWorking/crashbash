---
id: C003
kind: claim
status: holds
created: 2026-08-21
tags: boot,provisioning
depends: tools/provision.py#provision
reconfirmed: 2026-08-22
verified_at: 2026-08-22 18:30:47
---

## Claim

Crash Bash provisioning selects CLI > environment > .env > one drop-in, refuses an invalid authoritative source or ambiguity, and publishes the executable plus BOOT and MENU only after the complete measured input set verifies

## Evidence

tests/test_provision.py passes 12 positive/mismatch/refusal tests through tools/provision.py, including a failing case for each of the 11 executable manifest facts and refusal of mutated, malformed, or unknown loaded-module facts. The real USA CHD passes SYSTEM.CNF 1/1, executable identity/header 11/11, and BOOT/MENU module identity 14/14.

## What would falsify it

A source can silently fall through, any input is published before the complete set verifies, a tracked executable or module field is not checked, or the real selected disc no longer provisions 11/11 executable and 14/14 module facts.

## Re-confirmed 2026-08-21

Post-landing recheck: tests/test_provision.py passed 8/8 through the tracked shipping provisioner; real USA media had already passed SYSTEM.CNF 1/1 and executable identity/header 11/11.

## Re-confirmed 2026-08-21

Post-landing crashbash_provision_selftest passed its resolution, refusal, identity, and atomic-preservation cases.

## Re-confirmed 2026-08-22

Post-commit provisioning selftest passed all 12 executable/module identity, refusal, unknown-field, and publication controls; the preceding real-media gate passed executable 11/11 and modules 14/14.

## Re-confirmed 2026-08-22

Post-commit provisioning selftest passed all 12 executable/module identity, refusal, unknown-field, and publication controls; the preceding real-media gate passed executable 11/11 and modules 14/14.
