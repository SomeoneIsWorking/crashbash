---
id: I005
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

Crash Bash tools/verify_boot.py port-side boot-boundary verifier

## Validated by

It launched the real Clang-built port against the verified executable and accepted all five required boundary facts while excluding both forbidden patterns. It then rejected a controlled trace with guest-main reachability removed and a controlled trace with an injected recomp miss.

## Known failure modes

This verifier judges a bounded port trace, not guest memory equality against a true oracle. It cannot establish correctness beyond the first CD/VSync stop and must not be cited as an oracle comparison.
