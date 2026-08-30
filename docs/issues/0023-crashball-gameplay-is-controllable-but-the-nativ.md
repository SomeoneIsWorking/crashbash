---
id: 23
title: Crashball gameplay is controllable but the native path omits briefing, HUD, and character sprites
status: investigating
symptom: The exact 3740-frame Crashball replay shows objective text, controls, HUD counters, character portraits, and ship occupants on the PSX diagnostic path; the shipping native path renders the arena, ships, and balls but omits those 2D layers
state_items: S004
tags: graphics,sprite,hud,ui,native-renderer,crashball,flow
created: 2026-08-30
updated: 2026-08-30
---

## Measured boundary

Exact build `a3a5fbd+psxport-a0c18b9e` replays the tracked 3,740-frame
`replays/flow/crashball-control.pad`. The PSX diagnostic path shows the objective page at frame 2500,
the controls page after the first additional Cross edge, and a live match after the second. Held Left
for frames 3560..3619 moves the player ship to the lower-left; held Right for frames 3620..3739 moves
it to the lower-right. The shipping native path consumes the same complete replay and exits cleanly,
but its final frame contains only the arena, ships, and balls.

## Root cause

The only shipping producer reached in this run is the fixed-model producer rooted at guest
`0x80019F1C`. The omitted layers still exist only as guest GPU output, which the native render path
deliberately does not replay because post-projection packets cannot support native depth, widescreen,
or interpolation. This is a missing game-state producer family, not an input or flow failure.

The first family is now attributed. Crash Bash allocates two packet pools from the guest heap, so the
old fixed-window tracer was structurally blind. Retail setup functions `0x800274FC`/`0x800276C4`
publish live bounds through six globals; the framework now resolves those descriptors without
hardcoding one observed heap address. On the exact objective frame, all 108 direct-pool rows acquire
emitters. `0x8002992C` dominates with 92 opcode-`0x3C` textured Gouraud quads; its pre-packet arguments
carry the texture descriptor, screen position, OT bin, and four colors. See
`docs/findings/crashbash-packet-pools.md`.

The retained-super override at `0x8002992C` now captures those authored inputs before the retail body
allocates or writes its packet. A pure decoder records immutable textured Gouraud quads in the
current scene snapshot, and the native producer emits only records addressed to the render list being
presented. It reads no guest packet or OT contents, GP0 stream, VRAM output, or framebuffer. The exact
2,500-frame Crashball replay reconciles every frame with no dropped layer and reports 51,318 native
quads over 1,283 frames; the native capture restores the objective instructions and top score digits.

## Remaining work

Attribute and own the still-missing 2D elements through the remaining direct-pool families:
`0x80029D28` (six rows), `0x80018B08` (five), and `0x8001A0D8` (five). Use the PSX objective frame only
as an independent visual/differential oracle; do not reconstruct product input from OT, GP0, VRAM
output, or the diagnostic framebuffer.
