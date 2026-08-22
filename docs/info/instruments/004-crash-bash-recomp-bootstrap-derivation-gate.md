---
id: I004
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

Crash Bash tools/recomp_bootstrap.py executable-to-substrate derivation gate

## Validated by

The shipping path accepted the real verified executable plus measured BOOT and MENU modules, emitted
1,063 roots into 1,724 resident/loaded functions, and checked the required CRT0, dispatch, index,
override, overlay-table, range, and identity interfaces. Its 9/9 selftest also rejects changed program
facts, an explicit seed outside its image, a module mutation, inconsistent generated declarations, and
a changed generated source whose input hash is otherwise unchanged. The cache records and verifies a
digest over every shipping generated source/interface, so existence alone is not treated as completeness.

## Known failure modes

Static pointer/table and direct-call discovery cannot see every runtime function pointer. Such a target
must first appear as an observed recomp miss and receive independent static confirmation before it
enters the explicit seed file. The instrument proves emission and configuration, not hardware behavior
or oracle agreement.
