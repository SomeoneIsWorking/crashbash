---
id: 17
title: The attract flow advances through runtime-patched computed dispatch boundaries, and the authored key ord carrier must be uniform in the key
status: open
symptom: Raising the frame cap past 301 fatal-aborted the render (OT key->ord non-injective), then fail-fasted on recomp-MISS 0x80016F34 and 0x8001DFF8; a 1200-frame probe now completes exit 0 and the attract sequence visibly advances past the EUROCOM logo
state_items: S002, S004
tags: attract,flow,dispatch,seeds,keyord,ot,authored-order,d32
created: 2026-08-29
updated: 2026-08-29
---

## What the 1200-frame probe exposed

The 301-frame gate never exercised the flow past the EUROCOM logo. With
`PSXPORT_NATIVE_FRAMES=1200` three gates fired in sequence, each fixed in the
same session (build `12641bb` + psxport `02430b1b`):

1. **keyord FATAL at key 3** — the producer's `pzToOrd(key*2)` carrier
   saturates every key nearer than the near plane (legal GTE output; the GTE
   has no near clip) into ord 1.0, so key->ord is not injective and
   `rq_apply_ot_lifo_depths` aborts. LESSON: pzToOrd's near clamp makes it
   unusable as an authored-key carrier.
2. **keyord FATAL at bucket 789** after a 1/pz-shaped retry — injective, but
   the 1/pz shape compresses far keys until bucket 789's 59 LIFO ties do not
   fit 4e-7 of D32 range (~6 distinct values). LESSON: the carrier must also
   leave every bucket room for its ties; affine-in-1/pz cannot, because
   adjacent-key band width shrinks as 1/pz^2.
3. **Final carrier** (`fixedModelSortKeyOrd`): linear in the key over the
   game's own domain `[0, depthLimit)` — the same domain the far rejection
   uses — half-key centered. Uniform bands give every bucket thousands of D32
   tie slots. The key IS the authored order; per-object depthBias is already
   baked into the key and must NOT re-enter the carrier (it broke frame-wide
   monotonicity across objects).

## The dispatch boundary family

`0x80012420` (`jalr $v0`, target loaded from object field s0+0x14) is a
computed dispatch whose method table the game patches at runtime: the pointer
slots hold 0 in the FILE (e.g. 0x8005064C) and real entries in RAM, so
`pointer_table_funcs` can never see them. Each live boundary is grown into
`main` seeds only from an observed recomp-MISS with the recorded call site:

- `0x80012678` (pre-existing seed)
- `0x80016F34` — reached at ~frame 300+, a0=0x801D4B58
- `0x8001DFF8` — reached after the key->ord fix, a0=0x801DAB70

Expect more members of this family as the flow advances further.

## Verified result

1200/1200 frames, exit 0, zero recomp-MISS, zero keyord fatals (probe
`scratch/logs/flow-probe6.log`). Frame-1000 present differs from frame 300 by
95.02% of pixels: the attract sequence is past the logo. Still the same guest
process state (0x8004E0B8) and app mode (0x80078C90) — the BOOT-scene loop is
advancing internally, and the menu transition has not been observed yet.

## Next step

Raise the probe (2400 frames) and find the next boundary — either another
`0x80012420` family member or the app-mode transition into the menu scene.
Then map what "remove the loading screens" needs: the load path the game
already uses (`load file start/done loading` synchronous front-loads) is where
a streaming/deferred replacement would go, AFTER faithful flow progression.
Do not remove loads by skipping the guest's load calls — the module loads are
load-bearing (BOOT/MENU/DAT are real code).
