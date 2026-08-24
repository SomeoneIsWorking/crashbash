---
id: C012
kind: claim
status: falsified
created: 2026-08-22
tags: cdc,timing,first-frame
depends: psxport.pin, tools/verify_command_response_timing.py#diagnose, tools/verify_boot.py#judge
reconfirmed: 2026-08-24
verified_at: 2026-08-24 19:37:55
falsified_on: 2026-08-25
---

## Claim

Pinned psxport d2266f4b still delivers GetTN before Crash Bash enters its caller poll state

## Evidence

The Clang product identifies itself as `522689f+psxport-d2266f4b`. Its 136-line real-disc boot gate
executes BOOT and nested MENU, reaches resident 0x8002DE2C, and presents no game frame. A bounded
register trace records GetTN, handler reads `02 01 01`, and acknowledgement at 0x8003E14C before
1,500 empty interrupt-flag polls at 0x8002DE2C; the validated command-order diagnostic accepts it.
Ghidra confirms the guest returns from command sender 0x8002E000 and stores the next state before
calling the poller, so synchronous shared-controller response availability is the missing timing.

## What would falsify it

A product built at the recorded pin exposes the GetTN response to 0x8002DE2C before handler
acknowledgement, advances beyond this state into a presented game frame, or no longer executes the
measured send-return-store-poll guest sequence.

## Re-confirmed 2026-08-24

Post-commit audit at recorded d2266f4b: command-order controls passed and the recorded bounded trace still shows GetTN drain and acknowledgement before 1,500 empty caller polls.

## FALSIFIED 2026-08-25

The recorded framework pin moved from eager-command d2266f4b to landed phase-machine 8611d756; C012 remains historical pre-fix evidence but no longer describes the current pinned product.

> Anything that cited this claim as proof must be re-checked. Grep the repo for it.
