---
id: I013
kind: instrument
status: trusted
created: 2026-08-28
---

## Instrument

Crash Bash retained-super model packet identity diagnostic

## Validated by

Focused test scans two allocation blocks against two targets (4 comparisons/1 match) and a zero-match case (2/0); exact product frame 299 reports 13 blocks/26 comparisons/0 matches and frame 300 reports 10/20/2, mapping nodes 0x800C2FF4 and 0x800C8D84 to their decoded source faces.

Its frame-300 identity mapping also made a falsifiable product prediction: when Crash Bash selected
frame-wide Authored ordering, the mapped red face should replace the mapped dark face at display
`(56,115)`. Exact PID 3898998 completed 301/301 reconciled frames and the independent queue/present
probe observed shipping and source OT both select red node `0x801E18B0` (the native source identity
corresponding to retained packet `0x800C8D84`), with output `(105,345)` changing from `(33,0,66)` to
`(247,41,74)` versus PSX `(255,41,82)`. The same full-frame capture still shows large black angular
starfield holes, proving the instrument can isolate its named face pair without incorrectly declaring
the entire picture repaired.

## Known failure modes

The diagnostic identifies retained-super allocation blocks and source faces; it does not certify
whole-frame coverage, absent submitter families, or 4:3 image parity. A zero match is meaningful only
with the reported target/block/comparison denominators.
