---
id: 23
title: Crashball briefing parity remains partial after native HUD and character ownership
status: resolved
symptom: The native objective page now owns text, HUD, overlay, side characters, and center arrows, but its two side objective markers remain dim teal/gold where retail renders bright yellow
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
Its flat-color twin at `0x80029D28` shares the same authored geometry/texture recipe and now carries an
explicit flat-shading record through that boundary. At stable pre-transition frame 2480 it restores
all four top portraits and the two lower markers while retaining the text; the run reconciles every
frame and records 3,091 native flat quads over 1,329 frames.

The five objective-frame `0x8001A0D8` calls all take its authored-screen branch. One source quad is
the briefing dimmer and four are the yellow frame; all use four copied XY/RGB vertices and title-owned
offset/scale/fade/blend/bin state. Their native overlay-layer owner restores the dimmer and frame while
preserving the HUD portraits and text. The later resolution below corrects that initial layer choice
without changing the decoded inputs. The 2,480-frame run emits 5,830 native quads from this branch
over 1,395 frames and again reconciles every frame with no dropped layer.

The cached/model residual is also attributed. Retail pixel provenance at `(30,105)` selects packet
`0x8014814C`; its writer is `0x800193A8`, called by model decoder `0x80019A60`. The enclosing block
starts at `0x801479CC`, and the packet is source face 48 of object `0x801CDAB0`, asset `0x8005445C`,
model data `0x800DBF98`, frame family `0x4000`. Before the fix, retail topology declared 198 faces but
the native recipe decoder produced zero because indexed animation frames had no source resolver.

The `0x4000` path selects a group descriptor, follows its `+0x0C` relative redirect to the frame
record, then expands the animation bank's 16-bit vertex indices into six-byte XYZ records while
preserving each index's low two flag bits. The same source recipe also covers family `0x1000`; the
separate interpolation branch remains explicitly refused. After the fix, face 48 projects to retail
SXY `(20,108)/(30,107)/(36,102)` exactly and retains authored sort key 542. A stable 2,480-frame run
emits 4,265,746 native faces under `0x80019F1C`, reconciles every frame, and drops zero layers.

## Resolution

`0x80018B08` is viewport/camera/render-list setup: it installs draw environment, screen offsets, GTE
projection/matrix state, render-list pointers, depth policy, and fade. It is not another drawable
producer. Indexed model ownership restores both side characters and the top/bottom center arrows.
Matched replay/frame evidence now identifies the remaining mismatch exactly. Retail display frame 2500
pixel `(125,70)` is packet `0x80159FA4`, model object `0x8009AC04`, face 178, CLUT row 335. Its cue
factor is zero and its packet colors equal the raw source colors, ruling out DPCS. Native initially
rendered the newer CLUT-row-336 phase; deferring fixed-model submission to
`SceneSnapshotHistory::presentable` selects row 335. Quantizing raster XY to verified packet SXY also
restores the thin face's coverage.

The remaining dim result was compositing: native pixel provenance showed the correct opaque model
winner, then the semi-transparent `0x8001A0D8` screen-color quad in `RQ_OVERLAY` painted afterward.
Retail pixel provenance showed the model packet painted after that dimmer. The decoder now marks that
authored-screen family for the keyed world ordering domain while ordinary textured sprites remain HUD.
Its existing source bin supplies the same frame-wide key/ord mapping as fixed models, so dimmers and
foreground fades interleave by title policy instead of by a hard layer split.

The exact 2,502-frame replay exits cleanly. At native display frame 2500, the semi quad is key 192 / ord
`0.994125366` and the opaque objective marker is key 128 / ord `0.996078491`; both are authored-depth
world items, so the farther dimmer cannot repaint the marker. The presented capture restores both side
markers to bright yellow like retail. This resolves the Crashball briefing residual without a
marker/color special case or any guest OT, packet, GP0, VRAM, or framebuffer product input.
