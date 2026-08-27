---
id: C020
kind: claim
status: holds
created: 2026-08-27
tags: graphics,textures,native-renderer
depends: game/render/model_recipe_capture.cpp#captureFixedModelRecipe, game/render/native_model_producer.cpp#submitFixedModel
---

## Claim

Crash Bash's fixed-frame source records provide a live descriptor-run UV, texture-page, and CLUT path
that contributes textured faces to the native 4:3 title scene.

## Evidence

Ghidra decompilation of retail `0x80017EE8` grounds the per-face UV-index stream, the descriptor's
six-bit run length and two texture-table forms, UV base, CLUT, and texture-page material override. A
serialized real-disc frame-255 native run from the restored shipping binary (exact PID 3121483)
captured 1,542 fixed faces including 382
textured, submitted 1,010 total, and returned cleanly. The 960x720 capture is 627672/691200 non-black.
Compared with the retained untextured run's 627072/691200 capture, visual inspection shows three new
sampled gradient squares and additional particle points around the same green title fragment. The
serialized PSX leg (exact PID 3090121) remains 668153/691200 and does not yet match the native picture.
The restored witness completed all 256 frame fences and its full log contains no guest-VSync
timeout/violation, watchdog fault/stall, fatal renderer error, or recompilation miss.

## What would falsify it

A repeatable frame-255 native run decodes zero textured faces, removing the textured branch leaves the
same pixels, source-level RE disproves the descriptor/UV/material mapping, or the apparent textured
fragments are shown to originate from an unclaimed guest producer rather than `submitFixedModel`.
