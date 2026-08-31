---
id: 25
title: Crash Bash FPS60 lacked a title-owned interpolated presentation
status: resolved
symptom: With PSXPORT_FPS60=1 the extra present was a verbatim replay of the previous queue: tier1=0 every frame, so motion still stepped at 30 Hz
state_items: S006
tags: render,fps60,interpolation
created: 2026-08-30
updated: 2026-08-31
---

## Cause and resolution

The framework was pacing an extra present, but Crash Bash supplied no title-owned interpolation
presentation. `CrashBashRuntime` now creates `InterpolatedScenePresentation`, which rebuilds matched
model items from two immutable scene snapshots and preserves real non-model submissions. GPU draw
environment is sampled at the real submission boundary so the midpoint cannot read the next
framebuffer's offset/clip state.

The clean 3,741-frame controlled replay reports 6,159,397 interpolated primitives across 3,701 extra
presents with no fatal, watchdog, recompilation miss, or VSync failure. Direct captures around frames
3698–3699 show midpoint ship/ball positions between their surrounding real frames, without duplicate
30 Hz motion.
