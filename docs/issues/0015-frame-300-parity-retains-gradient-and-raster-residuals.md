---
id: 15
title: Frame-300 parity retains small residuals after exact DPCS, UV, Gouraud, and untextured coverage
status: resolved
symptom: Frame-300 raster residuals traced five shared Vulkan/PSX boundaries; the last one-pixel difference is below the project's evidence bar
state_items: S004
tags: render,texture,uv,raster,gradient,parity
created: 2026-08-29
updated: 2026-08-30
---

## Measured boundary

The native producer correctly renders `face.retailColors`, the retail `applyModelDpcs` result. That
earlier change reduced the frame-300 diff from 106,795 to 105,978 pixels, but it exposed a subtitle
coverage mismatch. Later packet-quantized SXY work reduced the current pre-UV-fix image to 98,280.

The remaining subtitle mismatch was not a DPCS defect. Framework commit `d6c51535` makes Vulkan use
the same affine-UV round-to-nearest rule as the PSX software rasterizer. On the same exact frame-300
capture, with the PSX diagnostic path retained as the reference:

| region | before UV fix | after UV fix |
|---|---:|---:|
| upper background (`y < 420`) | 46,284 | 46,332 |
| subtitle (`420 <= y < 490`) | 21,512 | 14,270 |
| lower background (`y >= 490`) | 30,484 | 30,304 |
| **whole frame** | **98,280** | **90,906** |

The subtitle band improves by 7,242 pixels and the full frame by 7,374. At source display pixel
`(61,145)`, native VRAM changes from `(136,112,160)` to `(232,208,248)`, matching retail.

The largest remaining gradient witness, display pixel `(111,25)`, then bound to untextured G3 packet
`0x800C397C`: object `0x800A0C74`, frame `0x200B`, face 94, material `0x0225`. Packet/native SXY and
all three captured colors agree exactly; GP0 draw mode `0xE1000000` proves dither is off. Retail emits
5-bit RGB `(3,0,6)`, while the ordinary Vulkan shader emitted `(4,0,7)`. Framework `9d370b06`
preserves the queue item's Gouraud/DTD state and applies PSX `round8 -> optional dither -> truncate5`
instead of rounding the interpolated float directly to 5-bit.

| region | after UV fix | after Gouraud fix |
|---|---:|---:|
| upper background (`y < 420`) | 46,332 | 14,399 |
| subtitle (`420 <= y < 490`) | 14,270 | 4,313 |
| lower background (`y >= 490`) | 30,304 | 6,671 |
| **whole frame** | **90,906** | **25,383** |

The source pixel is now `(24,0,48)` and presented pixel `(25,0,49)`, both exact retail matches. The
shipping GPU discriminator changed from the measured failure `1C04` versus expected `1803` to an exact
pass, and separately proves the negative/positive DTD cells as `3DEF`/`4210`.

The next residual witness, source display pixel `(85,137)`, binds to retail packet `0x800C89EC`:
object `0x801E18B0`, frame `0x2001`, face 27, material `0x002C`, with exact packet/native SXY
`(90,127)/(85,137)/(90,138)` and exact white colors. Retail includes the second vertex and writes
white; Vulkan missed it and exposed the darker preceding face because PSX coverage samples integer
pixel coordinates while Vulkan samples at half-integer pixel centers. Framework `3b033259` shifts
only the ordinary untextured vertex path by half a native pixel.

| region | after Gouraud fix | after untextured coverage fix |
|---|---:|---:|
| upper background (`y < 420`) | 14,399 | 466 |
| subtitle (`420 <= y < 490`) | 4,313 | 2,829 |
| lower background (`y >= 490`) | 6,671 | 2,251 |
| **whole frame** | **25,383** | **5,546** |

The shipping 2x GPU edge discriminator shows the other answer: unshifted coverage resolves to
`1C04`, while PSX-centered coverage resolves to the expected `350B`. Applying the same transform to
the textured path was tested and rejected because it worsened the whole-frame diff to 15,053.

## Proven cause and falsified hypothesis

Retail packet `0x800C84D4` is opcode `0x36`, a semitransparent textured Gouraud triangle from object
`0x801E1DB8`, frame `0x2008`, face 9, material `0xA1B2`. Its three packet SXY values exactly match
the native face. Raw colors are `0x00C6C6C6`; captured modeled colors and all three packet colors are
exactly `0x006C6C6C`. The adjacent face-8 packet has the same exact color and geometry agreement.
This falsifies the former claim that the subtitle used incomplete or different DPCS inputs.

The software-GPU write chain at absolute VRAM pixel `(61,401)` names only the expected opaque
background writer and packet `0x800C84D4`; its blend mode, incoming RGB, before value, and after value
are correct. Native Vulkan instead selected palette texel `0xC210` from the adjacent gray sample where
the PSX affine rasterizer rounded to the white texel. `psx_uv.glsl` had reconstructed integer-pixel
phase but truncated fractional UV. The shipping GPU selftest now reaches every port and passes a
28/28 matrix covering 1x/3x, opaque/semitransparent, and positive/negative integer and fractional
slopes; the independent blend matrix remains 16/16.

The next witness, source display pixel `(238,61)`, then bound to retail packet `0x800C7454`: a dark
textured face over an underlying G3 face, both with exact native/packet geometry and colors. Retail
samples texel `0x8421`, truncates its modulation to zero, and leaves background `0x0C01` unchanged; the
native result was `0x1022`. Framework `db30e329` makes both Vulkan textured paths compute the PSX's
`(texel5 * color8) / 128` with an integer truncating divide instead of rounding the float product back to
5-bit, and adds `gpu_vk_modulation_selftest.cpp` as its own three-case discriminator.

| region | after untextured coverage fix | after modulation fix |
|---|---:|---:|
| upper background (`y < 420`) | 466 | 39 |
| subtitle (`420 <= y < 490`) | 2,829 | 0 |
| lower background (`y >= 490`) | 2,251 | 6 |
| **whole frame** | **5,546** | **45** |

The 45-pixel residual then resolved to seven source pixels, and binding two of them named one cause.
Source `(206,82)` is covered by object `0x801D0AC8`, frame `0x2019`, faces 0 and 3 — TEXTURED triangles
two pixels wide, whose vertices are `(206,83)/(206,81)/(207,82)`. Source `(320,63)` is covered by a
semitransparent textured face of frame `0x200C`, material `0xA092`, over an untextured base whose retail
writer `0x800BEF5C` and native producer agree exactly. Both differing pixels sit on the edge of a tiny
textured primitive, and the model layer underneath matches retail exactly in geometry and color.

The cause is that only the untextured pipeline carried the PSX coverage rule. `tri.vert` shifted by half a
native pixel so PSX integer-coordinate coverage lands on Vulkan pixel centers; `tritex.vert` did not. On a
two-pixel-wide primitive that is the difference between covering a pixel and missing it. Framework
`0e6c7e5d` applies the same shift to the textured pipeline and rewinds affine UV from the native pixel's
center rather than its corner, so geometry and UV express one convention. This supersedes the earlier
rejection of the textured shift: that attempt kept the corner-relative rewind, double-correcting by half a
pixel, and was measured before the modulation fix.

| region | after modulation fix | after textured coverage fix |
|---|---:|---:|
| upper background (`y < 420`) | 39 | 6 |
| subtitle (`420 <= y < 490`) | 0 | 0 |
| lower background (`y >= 490`) | 6 | 0 |
| **whole frame** | **45** | **6** |

Frames 299/300/301 fall from 99/45/24 to 0/6/6. Frame 299 is exact.

## Closed by directive

USER 2026-08-30: "Change the directive, pixel matching doesn't matter. I just want working game that
looks correct."

One source pixel of frame 300 still differs — `(118,209)`, native `(11,0,15)` versus retail `(11,0,16)`,
a single 5-bit step in blue spread over six display pixels by the 960x720 upscale. That is below the
project's evidence bar and is not work. This issue is resolved: the five shared raster boundaries it
found are fixed and each carries a shipping discriminator, and the frame comparison it established
remains available as a diagnostic for locating a VISIBLE defect. Do not reopen it for a difference count.

## Where

`external/psxport/runtime/recomp/shaders_gpu/psx_uv.glsl`,
`external/psxport/runtime/recomp/gpu_vk_texture_phase_selftest.cpp`,
`external/psxport/runtime/recomp/shaders_gpu/tri.frag`,
`external/psxport/runtime/recomp/shaders_gpu/tri.vert`,
`external/psxport/runtime/recomp/gpu_vk_untextured_selftest.cpp`,
`external/psxport/runtime/recomp/gpu_vk_modulation_selftest.cpp`,
`external/psxport/runtime/recomp/gpu_vk_texture_coverage_selftest.cpp`,
`external/psxport/runtime/recomp/shaders_gpu/tritex.vert`,
`external/psxport/runtime/recomp/shaders_gpu/psx_uv.glsl`,
`external/psxport/runtime/recomp/shaders_gpu/tritex.frag`,
`external/psxport/runtime/recomp/shaders_gpu/trisemi_hw.frag`,
`external/psxport/runtime/recomp/gpu_native.cpp`, `game/render/model_recipe_capture.cpp`,
`game/render/native_model_producer.cpp`, `game/render/model_packet_identity_diagnostic.cpp`.
