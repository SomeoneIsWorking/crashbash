---
id: C015
kind: claim
status: holds
created: 2026-08-26
tags: cdc,timing,interrupt
depends: tools/verify_cdc_phase_progress.py#judge, psxport.pin
---

## Claim

The recorded CDC phase candidate trace separates every measured Crash Bash Pause INT3 acknowledgement from its INT2 completion on a fresh IRQ-handler entry

## Evidence

At recorded pin 17981527 (CDC-identical to 8611d756), cdc_command_service holds phase-2 completion while the response queue is nonempty, and acknowledging a queued response increments irq_sequence for the next response. tools/verify_cdc_phase_progress.py accepts the 8,606-line candidate trace with 5/5 distinct Pause INT3/INT2 handler-entry pairs and passes 8/8 controls including a forced coalesced-response negative. This proves controller response-edge separation in the candidate trace, not the guest-visible async result 1 -> 0 on a clean landed run.

## What would falsify it

A clean pinned trace coalesces a Pause INT3 and INT2 into one 0x8003F5F0 entry, the pinned CDC code changes its queue-empty completion gate/fresh-edge behavior, or the verifier accepts a fixture with no second handler entry
