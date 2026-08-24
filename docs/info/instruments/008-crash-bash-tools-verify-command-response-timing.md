---
id: I008
kind: instrument
status: trusted
created: 2026-08-22
---

## Instrument

Crash Bash tools/verify_command_response_timing.py GetTN handler/caller ordering diagnostic

## Validated by

Accepted the real pinned-ad5cf802 trace with 1,827,971 empty caller polls after handler drain/ACK,
then rejected four controlled opposite answers: missing GetTN, wrong response bytes, missing handler
acknowledgement, and caller-visible response (5/5).

Revalidated against pinned psxport d2266f4b: accepted a bounded real trace with handler drain/ACK
before 1,500 empty caller polls; the normal selftest retained all 5/5 controlled answers.

## Known failure modes

(none recorded yet)
