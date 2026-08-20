---
id: C003
kind: claim
status: holds
created: 2026-08-21
tags: boot,provisioning
depends: tools/provision.py#provision
reconfirmed: 2026-08-21
verified_at: 2026-08-21 02:44:52
---

## Claim

Crash Bash provisioning selects CLI > environment > .env > one drop-in, refuses an invalid authoritative source or ambiguity, and publishes only an executable matching SYSTEM.CNF plus all 11 tracked identity/header facts

## Evidence

tests/test_provision.py passed 8/8 positive/mismatch/refusal tests through tools/provision.py, including a failing case for each of the 11 manifest facts; the real USA CHD passed SYSTEM.CNF 1/1 and executable identity/header 11/11 at the tracked SHA-256.

## What would falsify it

A source can silently fall through, wrong media can overwrite the verified output, a tracked identity/header field is not checked, or the real selected disc no longer provisions 11/11.

## Re-confirmed 2026-08-21

Post-landing recheck: tests/test_provision.py passed 8/8 through the tracked shipping provisioner; real USA media had already passed SYSTEM.CNF 1/1 and executable identity/header 11/11.
