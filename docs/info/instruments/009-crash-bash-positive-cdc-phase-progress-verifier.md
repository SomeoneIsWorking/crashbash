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
02/01/01, exact command denominators, zero former 0x8002DE2C polls, 306 sector events, both required
continuous ranges through LBA 17655, and 5/5 Pause INT3 acknowledgement / INT2 completion pairs under
distinct IRQ-handler entries. Its 8/8 selftest rejects a missing target, a recurring old poll, a
wrong response byte, a missing command, a forbidden runtime error, response coalescing with no second
handler entry, and a subprocess that times out before the positive target. The timeout case starts a
real child and proves the runner terminates that exact `Popen` child while returning REFUSED rather
than success.

## Known failure modes

The gate proves the CDC handoff, bounded later disc progress, and controller-level response-edge
separation. It does not prove the guest-visible read-completion result, a first game frame, input,
audio, or gameplay, and it must be rerun against the clean executable built at the recorded framework
pin before the candidate trace can be promoted to landed-product evidence.
