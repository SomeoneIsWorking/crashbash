---
id: I007
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

Crash Bash `tools/verify_read_completion.py` true-Beetle/port 189-sector completion comparator

## Validated by

Accepted a paced port sequence with initial result 189, completion-pending result one, and settled
result zero. Rejected instant completion before the read-start return, a repeated 189-sector start,
wrong async state, and a completion-pending state that no sync call returns (5/5). On the actual USA-disc traces it accepts the independent Beetle
contract and rejects psxport `692b9b20` because the only four read returns are `-1` after completion.
It also rejected the first pacing near-miss, which returned 188 after one synchronous INT1 and never
exposed the oracle's completion-pending result one.

It rejected the landed psxport `3418a79b` trace as well: the port now returns the initial
189 and reaches completion, but its completion-pending one is only an internal state-word transition;
neither measured sync/poll function returns that state before it clears. The parser accepts appended
diagnostic fields such as guest cycles without weakening the required state contract.

## Known failure modes

The comparator consumes separately captured guest-state logs and does not create either trace. A
debugger probe that traps on every polling return can perturb the wall-clock watchdog, so capture must
use sparse boundaries or state-word watches. It proves this one read interval and result-state
sequence, not full-RAM lockstep or later gameplay.
