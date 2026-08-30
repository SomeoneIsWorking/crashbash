---
id: 15
title: Frame-300 parity retains smaller raster residuals after exact DPCS, UV phase, and Gouraud quantization
status: open
symptom: After correcting Vulkan affine-UV rounding and untextured Gouraud quantization, 25,383 of 691,200 frame-300 pixels still differ from the PSX reference by more than 8
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

## Remaining falsifier and next step

This issue stays open because 25,383 pixels still differ: upper 14,399, subtitle 4,313, and lower 6,671.
Select a representative from the new residual—not one already closed by UV or G3 quantization—and bind
its final writer and raster inputs before changing another shared rule. Do not tune colors, add per-object
exceptions, or alter DPCS.

## Where

`external/psxport/runtime/recomp/shaders_gpu/psx_uv.glsl`,
`external/psxport/runtime/recomp/gpu_vk_texture_phase_selftest.cpp`,
`external/psxport/runtime/recomp/shaders_gpu/tri.frag`,
`external/psxport/runtime/recomp/gpu_vk_untextured_selftest.cpp`,
`external/psxport/runtime/recomp/gpu_native.cpp`, `game/render/model_recipe_capture.cpp`,
`game/render/native_model_producer.cpp`, `game/render/model_packet_identity_diagnostic.cpp`.
