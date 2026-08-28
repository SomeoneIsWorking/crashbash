---
id: C021
kind: claim
status: holds
created: 2026-08-28
tags: graphics,native-renderer,ordering
depends: game/render/model_packet_identity_diagnostic.cpp, external/psxport/runtime/recomp/render_queue.cpp#RenderQueue::resolveKeyOrderFaces
---

## Claim

Crash Bash frame-300 damaged letter pixel is a cross-object native D32 reversal of the retail OT winner, not missing geometry or material decode

## Evidence

At q=(56,115), retained-super packet identity maps dark node 0x800C2FF4 to object 0x800A0C74/face33/material01D6/key3312 and red node 0x800C8D84 to object 0x801E18B0/face50/material003D/key3160; both are accepted opaque untextured Gouraud. Retail selects red. Shipping GREATER_OR_EQUAL selects dark D32 0.088171428 over red 0.083298709; authored key depths select red 0.082564623 over dark 0.081545500.

The diagnosis now has a product-level positive prediction. Exact PID 3898998 ran 301/301 reconciled
frames at build `87a4e75-dirty+psxport-36e8daa9` with Crash Bash's Authored factory default active.
At frame-299 display `(56,115)`, shipping and source OT both select red node `0x801E18B0`, sequence
884, key 3160. The output correspondent `(105,345)` changes from pre-fix native `(33,0,66)` to
`(247,41,74)`, versus PSX `(255,41,82)`, and visual inspection confirms the full EUROCOM letter edge
is restored without a new occluder. This confirms the named two-face ordering claim only; the broad
black angular starfield/background holes were a separate partial-parity defect in that witness. Later
PID 4054917 resolved those holes through the independent AVSZ3/ZSF3 ownership path; that result neither
strengthens nor falsifies this two-face ordering claim.

## What would falsify it

if a current post-H reference/native witness shows either named face does not cover q=(56,115), the
source-OT packet walk selects a node other than 0x800C8D84 there, or activating Authored ordering no
longer makes the shipping winner agree with that retail red face
