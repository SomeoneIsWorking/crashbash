---
id: I003
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

Crash Bash tools/provision.py disc and executable identity gate

## Validated by

The shipping provision path accepted a matching synthetic PS-X EXE, rejected a wrong SYSTEM.CNF boot path, produced a mismatch for each of the 11 independently altered manifest facts, rejected a one-byte executable mutation, refused extraction failure, preserved prior output on mismatch, and separately verified the real USA CHD at SYSTEM.CNF 1/1 plus executable facts 11/11.

## Known failure modes

The gate establishes identity only for the selected media; it does not prove physical-disc provenance,
recompilation, or boot. Hermetic negative cases replace `discdump` with a controlled extractor, while
the real-media leg validates libchdr/discdump only on the successful answer.
