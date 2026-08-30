---
id: 15
title: Frame-300 parity retains small residuals after exact DPCS, UV, Gouraud, and untextured coverage
status: open
symptom: After correcting Vulkan UV, Gouraud quantization, untextured coverage, and texture modulation, 45 of 691,200 frame-300 pixels still differ from the PSX reference by more than 8
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

## Remaining falsifier and next step

This issue stays open because 45 pixels still differ: upper 39, subtitle 0, lower 6. They form two small
clusters rather than a spread residual. The largest is display `(386..387, 252..254)`, where native
`(16,33,58)` faces retail `(8,0,33)`; the second is `(390..391, 249..251)`, native `(16,25,49)` versus
retail `(16,8,41)`. Bind one of those exact pixels to its final writer and raster inputs before changing
another shared rule. Do not tune colors, add per-object exceptions, or alter DPCS.

## Where

`external/psxport/runtime/recomp/shaders_gpu/psx_uv.glsl`,
`external/psxport/runtime/recomp/gpu_vk_texture_phase_selftest.cpp`,
`external/psxport/runtime/recomp/shaders_gpu/tri.frag`,
`external/psxport/runtime/recomp/shaders_gpu/tri.vert`,
`external/psxport/runtime/recomp/gpu_vk_untextured_selftest.cpp`,
`external/psxport/runtime/recomp/gpu_vk_modulation_selftest.cpp`,
`external/psxport/runtime/recomp/shaders_gpu/tritex.frag`,
`external/psxport/runtime/recomp/shaders_gpu/trisemi_hw.frag`,
`external/psxport/runtime/recomp/gpu_native.cpp`, `game/render/model_recipe_capture.cpp`,
`game/render/native_model_producer.cpp`, `game/render/model_packet_identity_diagnostic.cpp`.
