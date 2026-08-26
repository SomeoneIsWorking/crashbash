---
id: I010
kind: instrument
status: trusted
created: 2026-08-26
---

## Instrument

Crash Bash tools/inventory_render_anchors.py retail projection/control inventory

## Validated by

Its 12/12 in-memory selftest recognizes a projection/control positive and direct predecessor while
rejecting a GTE operation with no projected output, a memory-write-only body, a cache from a stale
selected recompiler, and a mismatched parsed-function denominator. Real runs reuse the shipping
recomp-bootstrap input/output hash owners and return exact psxport/recompiler provenance. They
independently reproduced both previous pin 17981527 (1,724 functions) and substrate baseline
99a42aa3 (2,005 functions) instead of silently treating either as the other.

## Known failure modes

This is a static generated-source inventory. It cannot prove that an anchor executes in a presented
frame, recover indirect callers, distinguish a game camera from a generic matrix helper, or establish
which game-state fields feed a producer. Runtime attribution and semantic decompilation remain
separate gates.
