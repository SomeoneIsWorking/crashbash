---
id: C008
kind: claim
status: holds
created: 2026-08-21
tags: boot,cdrom,harness
depends: tools/verify_read_completion.py#completion_contract, psxport.pin
reconfirmed: 2026-08-22
verified_at: 2026-08-22 14:13:51
---

## Claim

On psxport 692b9b20, Crash Bash repeated CRASHBSH.DAT because the controller completed all 189
sectors before the guest read-start function returned

## Evidence

Independent Beetle returned 1 from 0x8003470C, then 189 from 0x800348A8 and 0x80027790 while 189 sectors remained; after LBA35987 it exposed async result 1 then 0 and advanced to a different 16-sector read. On pinned psxport 692b9b20, live GDB showed 0x8003470C return 0 and 0x80027790 return -1 four times after dest 0x800D7490/expected 0x8C94 and active/remaining/async all zero, with no 0x800348A8 or 0x80027944 event. tools/verify_read_completion.py passed 5/5 and rejected the live port trace.

The deterministic guest-cycle framework now pinned at psxport `3418a79b` is the causal confirmation: its starter returns
one with all 189 sectors pending, the same range no longer restarts, `done loading` is printed, and
execution reaches the next unloaded entry at 0x80092BDC. Its separate completion-return mismatch is
tracked in issue 7 and does not restore the original retry.

## What would falsify it

A deterministic time-scheduled shared drive still repeats the same 189-sector range after restoring
the starter's result one with all sectors pending, or a repeated true-oracle trace changes the
measured start/completion results

## Re-confirmed 2026-08-21

Deterministic guest-cycle scheduling restores starter result 1 with all 189 pending and removes the same-range retry; live prints done loading and reaches 0x80092BDC.

## Re-confirmed 2026-08-21

Strict comparator 5/5 accepts returned pending 1->0 and rejects the landed 3418a79b live trace because pending 1 exists only inside IRQ handling; deterministic pacing still removes the original same-range retry and reaches 0x80092BDC.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b is the causal positive: starter returns 1 with all 189 pending and the same range no longer retries. The strict 5/5 comparator still rejects landed completion ordering because pending 1 is transient inside one IRQ drain.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b keeps the causal positive: starter returns 1 with all 189 pending and no same-range retry. Strict 5/5 comparator still rejects completion because pending 1 is transient inside one IRQ drain.

## Re-confirmed 2026-08-21

Post-landing deterministic pacing still removes the original same-range retry and reaches 0x80092BDC; the strict 5/5 comparator continues to reject the distinct transient-only pending-state ordering.

## Re-confirmed 2026-08-22

Pinned psxport 7f5d3f13 retained the causal positive at the observable boundary: the same 189-sector range did not retry, done loading printed, and execution reached 0x80092BDC; the strict completion comparator's 5/5 controlled-answer selftest still passed.
