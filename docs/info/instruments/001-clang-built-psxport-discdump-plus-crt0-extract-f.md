---
id: I001
kind: instrument
status: trusted
created: 2026-08-20
---

## Instrument

Clang-built psxport discdump plus crt0_extract for Crash Bash title measurement

## Validated by

discdump distinguished the configured SCUS_945.70 from seven other root/nested entries and extracted 69-byte SYSTEM.CNF plus 432128-byte executable; crt0_extract resolved 8/8 fields on the retail image and its selftest produced both complete and incomplete/refused answers across 59/59 checks

## Known failure modes

`discdump` measures the supplied image; it does not prove that image is the intended retail revision.
`crt0_extract` recognizes measured PSYQ startup shapes and must refuse or report incomplete on an
unsupported prologue. Neither tool proves that a future `GameConfig` wires the measured fields.
