---
id: 15
title: The subtitle object moves AWAY from retail under the captured DPCS cue model, and two gradient-band classes remain
status: open
symptom: After the native producer renders the captured retail DPCS colors, 2169 frame-300 pixels sit farther from the PSX reference than before, concentrated in the subtitle glyph interiors; ~106k pixels still differ from retail overall
state_items: S004
tags: render,dpcs,depth-cue,subtitle,parity,model-producer
created: 2026-08-29
updated: 2026-08-29
---

## Measured boundary (PID run at build `07ec0ce` + psxport `02430b1b`, 301/301 frames, exit 0)

The producer now renders `face.retailColors` (the capture's `applyModelDpcs(sourceColors, cue)`)
instead of the raw palette colors, so native output carries the depth cue retail bakes into its
submitted vertex colors. Frame-300 present capture A/B, same build, PSX diagnostic path
pixel-identical (`AE=0`) to the retained reference:

| metric | before | after |
|---|---|---|
| native vs PSX pixels differing >8 | 106,795 / 691,200 | 105,978 / 691,200 |
| pixels changed by this fix | — | 12,122 |
| …of which toward retail | — | 9,930 |
| …of which away from retail | — | 2,169 |

Net positive, and the EUROCOM logo shading now reads as retail DPCS-darkened. It is NOT whole-frame
parity. The residual 105,978 pixels decompose by band:

- **aurora** (y<420): ~48,273 px — smooth background gradient bands offset by one shade step
  (e.g. `(33,0,58)` vs `(33,0,49)`); dither/gouraud-interpolation class, not color identity.
- **lower background** (y>490): ~32,568 px — same gradient-band class.
- **subtitle** (420..490): ~25,137 px — systematic green +9 (e.g. `(255,25,8)` vs `(255,16,8)`),
  PLUS glyph-interior coverage where both the old and new native are dark while retail is bright
  (`(74,49,99)`/`(115,90,140)` vs retail `(230,214,255)`).

## The falsifier that keeps this open

The 2,169 worse pixels prove the captured cue model is INCOMPLETE, not merely noisy: for the
subtitle object, applying `applyModelDpcs` with the draw's captured cue darkens glyph interiors
that retail keeps bright. Either that object's retail cue inputs differ from what
`resolveModelColorCueInputs` captured (object-flag semantics at `0x80019F1C`/`0x8001DD50`), or
retail applies the cue through a mechanism other than baked vertex colors for that object.

## Next step

RE the subtitle object's retail submission at the packet level (extend
`tools/verify_render_anchor_reach.py`-style packet binding to the subtitle draws) and answer which
of the two mechanisms retail uses. Do not tune `applyModelDpcs`, add a per-object special case, or
clamp the cue — the capture model must say what retail says.

## Where

`game/render/model_recipe_capture.cpp` (retailColors), `game/render/native_model_producer.cpp`
(render colors), `game/render/model_material_diagnostic.cpp` (`applyModelDpcs`,
`resolveModelColorCueInputs`), `game/render/model_submit_capture.cpp` (cue inputs).
