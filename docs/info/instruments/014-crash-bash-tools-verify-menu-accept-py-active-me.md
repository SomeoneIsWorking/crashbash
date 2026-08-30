---
id: I014
kind: instrument
status: trusted
created: 2026-08-30
---

## Instrument

Crash Bash tools/verify_menu_accept.py active-menu action verifier

## Validated by

The 10/10 controlled judge rejects missing provenance, idle input, an idle DAT28136 successor, false START acceptance, missing START
input, a wrong pending table, missing Cross acceptance, a wrong DAT28136 callback, and a successor
callback that never executes. The real Clang product supplies the other answer with 88 idle/START
updates, three ignored START edges, one Cross edge scheduling `0x800B8E50`, DAT28136 registration
replacing app callback `0x80093038` with `0x800B4694`, and one observed entry to that update. Every run
must announce the same exact compiled-substrate identity. The instrument encodes the two-phase queue:
six updates retain current manager `0x800B8E28` while pending is already `0x800B8E50` before the old
callback stops, then requires the separately measured successor behavior.

## Known failure modes

This is a shipping-port differential against statically decoded retail control flow, not an emulator
lockstep. It proves the active type-`0x0101` menu action and the first DAT28136 successor callback,
not later scene completion or full state parity. It runs three finite product instances serially and
requires the verified executable plus a built generated substrate; default CTest runs only its
hermetic judge.
