---
id: I006
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

Crash Bash tools/verify_oracle_irq.py true-Beetle/port interrupt-order comparator

## Validated by

Accepted the independent 600-frame Beetle interpreter plus actual-CHD trace against the live port trace, then rejected a wrong oracle dispatcher, a changed saved stack, and a missing port CD callback (4/4).

## Known failure modes

The comparator proves the saved interrupt re-entry prefix and the port-side CD callback order only. It
does not compare full RAM or later filesystem state, and its real comparison requires separately
captured logs from the independent Beetle interpreter and the port.
