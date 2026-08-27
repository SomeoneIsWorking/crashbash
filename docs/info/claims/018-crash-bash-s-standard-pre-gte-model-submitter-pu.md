---
id: C018
kind: claim
status: holds
created: 2026-08-27
tags: graphics,native-renderer,scene-snapshots
depends: game/render/model_submit_capture.cpp#recordIfRenderable, game/render/scene_snapshot.cpp#SceneSnapshotHistory::beginFrame
---

## Claim

Crash Bash's standard pre-GTE model submitter publishes non-empty title-owned scene snapshots during the real-disc title sequence.

## Evidence

Clang-built dirty candidate ran the USA disc for 300 native-owned frames and returned cleanly: override 0x80019F1C was hit 10,452 times and frame 255 contained 71 accepted ModelDraw records; a separate 350-frame run reached the same override 17,004 times. Neither run reported guest VSync, timeout, watchdog, recompilation miss, or fatal. SceneSnapshotHistory rotation/storage selftest passed.

## What would falsify it

A real-disc title run reaches 0x80019F1C but records zero accepted draws, or semantic decompilation disproves the frame/flag/model guards or captured argument mapping.
