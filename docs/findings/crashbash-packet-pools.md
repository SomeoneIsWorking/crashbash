# Crash Bash heap packet pools and first 2D submitter

The retail renderer does not use a fixed packet arena. Ghidra decompilation of resident functions
`0x800274FC` and `0x800276C4` shows two calls to the guest heap allocator, each for
`requested_size + 0x1800` bytes. The functions publish inclusive base and exclusive end pointers at:

| parity | base pointer global | end pointer global | current pointer global |
|---|---:|---:|---:|
| 0 | `0x8005F790` | `0x8005F794` | `0x8005F798` |
| 1 | `0x8006379C` | `0x800637A0` | `0x800637A4` |

The exact objective-frame RAM dump at replay frame 2500 contained live bounds
`[0x801F97E8,0x801FEFE8)` and `[0x801F3FD0,0x801F97D0)`. Those are observed values, not title
constants. The tracked `GameConfig` facts therefore name the six descriptor globals, never those heap
addresses.

The OTs are separate static storage. `0x800275F8` clears 4,096 entries at `0x8005B790` and
`0x8005F79C`; `0x800272AC` alternates between them, clears the selected OT, and resets its current
packet pointer from the adjacent base global. The OT-page stride is `0x400C`.

## Objective-frame attribution

Before runtime pool descriptors existed, the exact frame-2500 `otattr` run walked 6,164 OT nodes but
recorded zero writer spans. With psxport's live-pool path and the six measured globals, the same replay
records 1,828 spans. Of the 2,068 OT rows printed by the bounded REPL report, all 108 rows inside the
active direct packet pool have an emitting function:

| emitter | rows | first GPU opcode | interpretation |
|---|---:|---:|---|
| `0x8002992C` | 92 | `0x3C` | textured Gouraud quad |
| `0x80029D28` | 6 | `0x2C` | textured flat-color quad |
| `0x80018B08` | 5 | `0xE3` | draw-area/state packet path |
| `0x8001A0D8` | 5 | `0xE1` | draw-mode plus untextured Gouraud quad path |

The other 1,960 printed rows are cached/model packets outside the direct heap pools; they remain a
different producer family rather than being hidden by one broad heap window.

Ghidra decompilation of `0x8002992C` identifies the pre-packet semantic boundary. Its arguments carry
the texture descriptor, packed screen position, OT bin, and four vertex colors; the function derives
UV/CLUT/tpage state and writes a `0x34`-byte packet. The retained-super capture now decodes those
source inputs into the immutable current-tick scene and the native producer restores the objective
text and score digits. It uses the title render-list identity for presentation routing but consumes no
packet or OT contents, GP0 stream, VRAM output, or PSX framebuffer.

`0x80029D28` is the flat-color twin of that leaf. It shares the descriptor, packed position, OT bin,
fade, blend, UV/CLUT/tpage, return, and render-list semantics, but takes one color and emits opcode
`0x2C`. Its retained super therefore feeds the same decoder with that color repeated across four
vertices and explicit flat shading. The stable frame-2480 native capture restores all four top
portraits and the two lower markers without disturbing the Gouraud text. The remaining direct rows
are `0x8001A0D8`'s draw-mode plus untextured Gouraud quad path and `0x80018B08`'s draw-area state path.
All five objective-frame `0x8001A0D8` calls take its authored-screen branch: one source quad spans the
briefing dimmer and four form the yellow border, at depth bias 384/bin 192. Its retained super now
copies the four XY/RGB vertices and title offset/scale/fade/blend/bin state into the native overlay
layer. The GTE branch remains unsupported rather than consuming projected output. `0x80018B08`
installs the draw environment, screen offsets, GTE projection/matrix state, render-list pointers,
depth policy, and fade; it is viewport/camera/render-list setup, not another drawable producer.

## Cached model residual

Retail pixel provenance at `(30,105)` selects packet `0x8014814C`. An OT-attribute watch identifies
`0x800193A8` as its writer and `0x80019A60` as the caller. The packet is face 48 of allocation block
`0x801479CC`, object `0x801CDAB0`, asset `0x8005445C`, model data `0x800DBF98`, frame family `0x4000`.
Retail topology declares 198 faces, while the native recipe previously decoded zero because it only
understood direct `0x2000` and indirect `0x5000` frame sources.

For `0x1000`/`0x4000`, the retail decoder selects a group descriptor, follows its `+0x0C` relative
redirect to the frame record, and expands the animation bank's 16-bit vertex indices into six-byte
XYZ records while preserving each index's low two flag bits. The native source resolver now owns that
same non-interpolated recipe; the distinct interpolation branch remains refused. Face 48 exactly
matches retail SXY `(20,108)/(30,107)/(36,102)` and authored sort key 542. A stable 2,480-frame run
emits 4,265,746 native model faces under `0x80019F1C`, restores the two side characters and the
top/bottom center arrows, reconciles every frame, and drops zero layers.

The remaining stable mismatch is the two side objective markers: retail renders them bright yellow,
while native output is dim teal/gold. Their source model/material state is the next attribution target;
GPU packets, OT contents, GP0, VRAM output, and the diagnostic framebuffer remain invalid product
inputs.
