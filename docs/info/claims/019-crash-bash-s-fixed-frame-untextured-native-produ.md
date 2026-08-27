---
id: C019
kind: claim
status: holds
created: 2026-08-27
tags: graphics,native-renderer,untextured
depends: game/render/model_transform_capture.cpp#modelTransformComposer, game/render/model_recipe_capture.cpp#captureFixedModelRecipe, game/render/native_model_producer.cpp#submitFixedModel
---

## Claim

Crash Bash's fixed-frame untextured native producer draws a non-black 4:3 title-scene subset from copied source model and transform state.

## Evidence

Serialized real-disc frame-255 A/B: native PID 3026346 captured 71/71 transformed fixed models, 1,160 source untextured faces, 717 submitted faces, and 627072/691200 non-black pixels; PSX PID 3026483 captured 668153/691200. Visual inspection shows the native animated background and a corresponding green title-letter fragment, while PSX additionally shows unsupported textured letters/effects. Both runs returned cleanly with no guest VSync violation/timeout, watchdog, recompilation miss, or fatal.

## What would falsify it

A repeatable frame-255 native run submits zero fixed untextured faces, becomes black, or source-level RE disproves the transform, topology, winding, or material mapping used by the producer.
