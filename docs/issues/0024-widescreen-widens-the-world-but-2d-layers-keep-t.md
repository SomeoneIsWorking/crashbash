---
id: 24
title: Preserve the Crashball briefing as one complete 4:3 presentation
status: resolved
symptom: The briefing overlay was centred at 16:9 while the arena behind it still widened, exposing non-oracle corner columns around an authored 4:3 composition
state_items: S005
tags: render,widescreen,hud,2d,authored
created: 2026-08-30
updated: 2026-08-31
---

## Cause and resolution

The objective-page `0x8001A0D8` screen-color quads are an authored 4:3 presentation signal, not a
request to stretch their dimmer/border across the enhanced arena. The first widescreen change moved
that overlay into the centre but still allowed models and sprites behind it to draw in the added
columns. Those visible corner regions are not part of the retail briefing.

`SceneSnapshot` now marks an authored-screen presentation. On those frames, both native models and
sprite/HUD submissions use the same centred native-width draw clip. The overlay is translated once,
not widened. The marker resets with the next simulation snapshot, so ordinary gameplay continues to
use the wide camera and full wide draw area.

At an actual 1280x720 sink, replay frame 2500 is the complete 4:3 oracle composition at full height
with only left/right bars: grey dimmer, all four yellow border strips, portraits, and all four life
counts are intact. Frame 3700 remains full-width, balanced 16:9 gameplay. The 4:3 path is unchanged.

The same presentation contract now covers boot and other upload-only 2D frames. The shared present
owner samples the native display width whenever the current frame has no native world geometry, so
the SCEA title card and loading/logo stills stay centered in a 4:3 viewport with black side bars.
Frames that submit native 3D continue to sample the widened target; this is a current-frame producer
fact, not a prior-frame or frame-aware interpolation decision.

## Rejected follow-up (2026-08-31)

A deterministic post-projection horizontal-FOV experiment was built against the same native replay,
using the existing wide target and draw-area extension. It produced a large untextured/incorrect
green triangle, duplicate character geometry, and blackened arena regions in the presented image;
the established wide image did not. The experiment was removed and is not evidence for changing the
current projection owner. Any future widening must change the engine-owned projection and clipping
contract together, preserving the single source vertex stream; do not add a post-submit coordinate
remap as a shortcut.
