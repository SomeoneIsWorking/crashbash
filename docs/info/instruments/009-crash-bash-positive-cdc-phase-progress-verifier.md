---
id: I009
kind: instrument
status: trusted
created: 2026-08-25
---

## Instrument

Crash Bash `tools/verify_cdc_phase_progress.py` landed-CDC positive-progress verifier

## Validated by

The shipping judge accepts the existing 8,606-line candidate trace with one GetTN response
02/01/01, exact command denominators, zero former 0x8002DE2C polls, 306 sector events, and both
required continuous ranges through LBA 17655. Its 7/7 selftest rejects a missing target, a recurring
old poll, a wrong response byte, a missing command, a forbidden runtime error, and a subprocess that
times out before the positive target. The timeout case starts a real child and proves the runner
terminates that exact `Popen` child while returning REFUSED rather than success.

## Known failure modes

The gate proves the CDC handoff and bounded later disc progress. It does not prove a first game frame,
input, audio, or gameplay, and it must be rerun against the clean executable built at the recorded
framework pin before the candidate trace can be promoted to landed-product evidence.
