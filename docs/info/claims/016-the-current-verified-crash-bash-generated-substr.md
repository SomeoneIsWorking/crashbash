---
id: C016
kind: claim
status: holds
created: 2026-08-26
tags: graphics,re,camera
depends: tools/inventory_render_anchors.py#build_inventory
---

## Claim

The previous verified-pin and current recorded-pin Crash Bash generated substrates both contain 31 static
projection anchors and 17 camera-control anchors, with resident 0x8001CD04 directly reaching
0x800193A8 and 0x8001AF2C, but their generated-function denominators and three anchor addresses differ

## Evidence

The provenance-bound analyzer reports previous pin 17981527/recompiler 2026-08-24.2 as 1,724
functions/31 projection/17 control and substrate baseline 99a42aa3/recompiler 2026-08-26.14 as
2,005/31/17. It names the two BOOT projection and one resident control-address substitutions while
retaining the resident chain. The 12/12 selftest rejects a stale selected recompiler and a mismatched
parsed denominator in addition to the static positive/negative fixtures.

## What would falsify it

Regenerating either exact substrate changes its recorded counts/edges, direct inspection of the
emitted bodies disagrees with the parser, or the selftest accepts a GTE-only, memory-only,
stale-provenance, or mismatched-denominator negative
