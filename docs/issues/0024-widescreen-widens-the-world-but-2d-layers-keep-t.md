---
id: 24
title: Widescreen widens the world but 2D layers keep their 4:3 extent
status: open
symptom: At 16:9 the briefing dimmer and yellow frame cover only a 4:3-sized region, and the HUD portraits and scores sit inboard instead of reaching the screen edges
state_items: S005
tags: render,widescreen,hud,2d,authored
created: 2026-08-30
updated: 2026-08-30
---

## Measured

Shipping native path, `PSXPORT_SETTINGS` with `aspect=1` (ASPECT_16_9), replay
`replays/flow/crashball-control.pad`, headless 960x720 sink.

The WORLD widens correctly: the warp room at frame 900 shows the third mode sign and more of both
side walls, and the Crashball arena at frame 3700 shows the full right-hand ramp that 4:3 cuts off.
Objects are not horizontally stretched, so this is a real FOV widening, not a present stretch.

Two 2D layers did not follow:

- **Authored screens keep a 4:3 extent.** The frame-2500 objective briefing draws its dimmer and
  yellow border over roughly the central 4:3 region of the widened frame, so the arena is visible
  un-dimmed to the left and right of the border. `scratch/screenshots/report/04-briefing-16x9-BUG.png`.
- **HUD is projected with the widened world instead of anchored to the screen.** The orange score
  digits span x 62..865 of 960 at 4:3 and only 80..768 at 16:9, so the portraits and scores move
  inboard and no longer sit near the corners.

## Why it matters

G002's success condition is that a wider aspect ratio does not change HUD intent. The world half of
widescreen is working; this is the remaining visible defect.

## Next step

Establish which producer positions each 2D family — the authored-screen decoder (`0x8001A0D8`), the
sprite-quad producers (`0x8002992C`, `0x80029D28`), and the HUD path — and give the screen-space
families a widened-frame placement rule instead of inheriting the world projection. Judge on the
running product at 4:3, 16:9, and 21:9.
