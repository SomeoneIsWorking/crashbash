---
id: I014
kind: instrument
status: trusted
created: 2026-08-30
---

## Instrument

Crash Bash tools/verify_menu_accept.py active-menu action verifier

## Validated by

The 7/7 controlled judge rejects missing provenance, idle input, false START acceptance, missing START
input, a wrong pending table, and missing Cross acceptance. The real Clang product supplies the other
answer with 88 idle/START updates, three ignored START edges, and one Cross edge scheduling
`0x800B8E50`. Every run must announce the same exact compiled-substrate identity. The instrument also
exposed and then encoded the two-phase transition: six updates retain current manager `0x800B8E28`
while pending is already `0x800B8E50` before the old callback stops.

## Known failure modes

This is a shipping-port differential against statically decoded retail control flow, not an emulator
lockstep. It proves the active type-`0x0101` menu action and queued target, not what the next manager
does after it becomes current. It runs three finite product instances serially and requires the
verified executable plus a built generated substrate; default CTest runs only its hermetic judge.
