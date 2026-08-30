---
id: 25
title: fps60 inserts a duplicate frame instead of an interpolated one
status: open
symptom: With PSXPORT_FPS60=1 the extra present is a verbatim replay of the previous queue: tier1=0 every frame, so motion still steps at 30 Hz
state_items: S006
tags: render,fps60,interpolation
created: 2026-08-30
updated: 2026-08-30
---

## Measured

`PSXPORT_FPS60=1` on the shipping native path with replay `replays/flow/crashball-control.pad`.
The framework accepts the request — `[fps60] TRUE per-object interpolated 60fps ON (source: env)` —
and does insert an extra present per frame at `t=0.500`. The queue classifier even marks the world
items eligible: `[fps60seq] rqcur layer=1 TIER1 n=1800 ... node0=801A9170`.

But every extra present reports zero interpolated prims:

    [fps60] f3710 slotA: replay prev=Q[N-1] n=3613 tier1=0 backdrop=0 t=0.500

`tier1=0` means `Fps60::tier1Render` contributed nothing, so the in-between frame is Q[N-1] replayed
verbatim. No `Fps60::tier1Render REFUSED` line appears, so the world-pass re-render is never even
attempted. The captured presents at frame 3700 with and without `PSXPORT_FPS60=1` are byte-identical.

The result is 60 Hz PACING of 30 Hz motion — the enhancement is wired but does nothing visible.

## Next step

Find why the tier1 world-pass re-render is not entered for this title (the `fps60WorldPass` hook and
the eligibility gate in `fps60.cpp`), then supply the native world pass so the mid-frame is rendered
from lerped camera and object state. Judge it on motion in the running game, not on a frame diff.
