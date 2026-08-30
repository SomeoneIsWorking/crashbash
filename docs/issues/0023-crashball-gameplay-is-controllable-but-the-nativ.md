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

## Next step

Attribute one omitted family at its game submitter, starting with the objective/HUD sprite path, then
decompile that owner and produce it from the source game state. Do not reconstruct it from OT, GP0,
VRAM output, or the diagnostic PSX framebuffer.
