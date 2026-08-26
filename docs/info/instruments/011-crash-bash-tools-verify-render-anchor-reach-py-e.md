---
id: I011
kind: instrument
status: trusted
created: 2026-08-26
---

## Instrument

Crash Bash tools/verify_render_anchor_reach.py exact live projection-attribution judge

## Validated by

Its 8/8 selftest accepts two exact known projection targets and reports their frame/row/call denominators, then refuses an unknown target, a zero jalr/inlined target, an empty histogram, and a recomp-MISS fatal. Real use additionally loads the provenance-bound shipping render-anchor inventory.

## Known failure modes

The framework emits no histogram before 50 presented frames. A zero decoded target means the caller
used `jalr` or retained an inlined/stale RA and this judge deliberately refuses rather than guessing.
Reaching an exact projection anchor proves execution ancestry only: it does not identify camera
fields, packet semantics, visible contribution, a native producer, image parity, widescreen, or
interpolation. The judge also depends on the generated cache and selected psxport inputs remaining
provenance-consistent; that refusal is intentional.
