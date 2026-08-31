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
