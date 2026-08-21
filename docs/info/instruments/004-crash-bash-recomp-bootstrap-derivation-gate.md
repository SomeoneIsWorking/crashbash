---
id: I004
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

Crash Bash tools/recomp_bootstrap.py executable-to-substrate derivation gate

## Validated by

The shipping path accepted the real verified executable, reproduced 339 roots / 907 resident functions and all required interfaces, and checked every shipping CRT0/GameConfig binding. Its controlled negatives changed one measured BSS bound, supplied an explicit seed outside executable text, and mutated one executable byte; all three were refused, so the instrument has demonstrated both answers.

## Known failure modes

Static pointer/table and direct-call discovery cannot see every runtime function pointer. Such a target must first appear as an observed recomp miss and receive independent static confirmation before it enters the explicit seed file. The instrument proves resident emission and configuration, not hardware behavior or oracle agreement.
