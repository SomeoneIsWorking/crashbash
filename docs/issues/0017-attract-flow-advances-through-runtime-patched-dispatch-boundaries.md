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

## 2400-frame probe result (same session)

The next boundary is NOT a seed: recomp-MISS 0x800C3434 (caller ra=0x80078D10,
jalr t9,v1 from BOOT code at 0x80078D08, c->pc=0x80078CA4). The diagnostic
attributes the slot to BOOT, but the words at 0x800C3434 do not decode as
R3000A code (opcode 0x3F = invalid) — the region is DAT-module payload loaded
OVER part of BOOT's range, the same nesting the seed file documents at
0x800B32B4 ("replacing this nested code range"), just further in. The slot
attribution by range is therefore AMBIGUOUS for nested modules, and the miss
class is "module not provisioned", not "function-discovery gap".

To land it: capture the CD/DMA trace (PSXPORT_DEBUG=cd) over a run that
reaches this load, identify the module's fixed load base covering 0x800C3434,
provision its image, and add it under `overlay_bases` — the same procedure the
first nested DAT module followed. Do not seed an address whose resident image
is absent; the seed would emit garbage.

## The DAT28272 module is provisioned; two carrier facts came out of it

The 2400-frame boundary was NOT a discovery gap. `crashbash-cd` names the load directly:
`38 sector(s) from LBA 28272 to 0x800B32B4` — a SECOND module in the same nested slot,
read over MENU once the attract flow leaves the menu. It is now provisioned as overlay
stem `DAT28272` (`titles/crashbash/dat28272_module.json`, 6/6 facts; provisioning is
20/20 across the three modules) and emits into the overlay table beside MENU with a
distinct 32-byte signature, which is what lets the router tell the two apart.

Two corrections to what this issue recorded earlier:

- **`0x800C3434` IS valid R3000A.** In the miss RAM dump it reads `27BDFFE8`
  (`addiu sp,sp,-0x18`). The "opcode 0x3F = invalid" reading came from BOOT's FILE
  bytes, not from the loaded image. The miss class was "module not provisioned"; it was
  never a decode failure, and the ambiguity note about nested slot attribution stands
  on its own without that claim.
- **The stem is named by disc LBA, not role.** `0x800C3434` registers a behavior vtable
  at `0x8005AA70` and the image's name table is animation states
  (BREATHE/JUMP/DAZED/WIN/TAZING), not MENU's front-end text — which rules OUT "second
  menu phase" but does not establish what it is.

### The authored key->ord carrier must not take ANY per-draw term

Provisioning the module put draws with two different `depthLimit` values in one frame for
the first time, and `rq_apply_ot_lifo_depths` fatal-aborted:

    FATAL: OT key->ord is not strictly monotone at key 187:
           ord 0.947654963, nearer band 0.728962779

Both ords are key 187: `0.947654963` is `1-(187.5)/3582` and the nearer band is the same
key under a limit of ~692. The OT is ONE frame-wide table ordered strictly by key, so a
carrier normalized by a PER-DRAW limit is not a function of the key at all — the same key
lands in different bands per object and frame-wide order inverts at the object boundary.
This is the same mistake as the per-object `depthBias` this issue already excluded from
the carrier, one step on: `depthLimit` is per-draw too.

`fixedModelSortKeyOrd` now takes the key ALONE (the `depthLimit` parameter is gone, so the
defect is a compile error rather than a runtime check) and normalizes by the key domain's
frame-wide bound from the game's own types: retail rejects every face whose key >=
depthLimit and depthLimit is a signed halfword, so no ACCEPTED key reaches `0x8000`.
`test_sort_key_ord_is_independent_of_any_per_draw_term` replays the measured 187/3582/692
pair and was proven to FAIL when the per-draw denominator is reintroduced into the shipping
function, and to pass when it is removed.

### Resolved: an OT bucket costs ONE depth value, because ties are a DRAW-ORDER question

With the carrier corrected the flow fatal-aborted one invariant later:

    FATAL: OT bucket 860 needs 472 distinct D32 ties
           between ord 0.973739624 and nearer band 0.973770142

That gap is exactly one uniform band. A band maps into the renderer's 3D depth range (7/8 of D32)
where a float32 ulp is 2^-24, so it holds ~448 distinct depths and bucket 860 needed 472. Reaching
f1023 before the carrier fix was not the old carrier being better — the per-draw denominators handed
some draws much wider bands while mis-ordering across objects.

Shrinking the key domain to fit would be a bandaid: the domain must bound every accepted key, and
`depthLimit` is a signed halfword, so any larger key would leave the 3D band silently.

The actual cause is that **depth resolution was the wrong currency**. Retail's AddPrim is head
insertion, so a bucket paints LIFO and the FIRST submission wins. Native draws in queue order under
GREATER_OR_EQUAL, so the LAST drawn wins — and `rq_apply_ot_lifo_depths` was buying the right answer
with one distinct depth value per tied face. But the depth buffer only has to say which BUCKET is
nearer; which face inside a bucket wins is a question about paint ORDER. So the bucket is now drawn
in reverse and shares ONE depth. Bucket size is unbounded; the "needs N distinct D32 ties" refusal is
gone because the situation it guarded cannot arise.

Draw order is permuted through a new `RqItem::draw_seq`, never `seq`. They were the same field, which
conflated two different things: `seq` is what the guest submitted, and the source-OT oracle
(`rq_source_ot_candidate_wins`) and fps60's pairing both reason about submission order, so
overloading it would have silently redefined their answers. `sortQueue` and the painter layer's
order checks now read `draw_seq`; everything reasoning about submission order still reads `seq`.
Because `draw_seq` comes from push()'s monotonic counter, a bucket's values are necessarily distinct,
and the resolver ABORTS on duplicates — an item that reached the queue without `draw_seq` seeded
would otherwise make the permutation a silent no-op that paints the bucket in the wrong order.

Verified end to end at `02430b1b`+ on the real disc:

- 2400/2400 frames, exit 0, zero keyord fatals, zero recomp-MISS, zero guest VSync timeout.
  The previous best was f1023 before a fatal.
- The tie path FIRES on real data: up to **1942 faces** in one frame's multi-face buckets, far past
  the ~448 the old mechanism could represent at all.
- No visual regression on the verified anchor: the frame-300 present is BYTE-IDENTICAL
  (sha256 `906f69c4ebfdbede...`) to the retained `native-final-300.ppm`, so the EUROCOM/logo result
  this issue's earlier work established is untouched.
- The flow genuinely advances: frame 1000 differs from frame 300 by 98.97% of pixels.
- psxport 121/121 and Crash Bash 23/23, including three keyorder/painter tests re-expressed to
  assert WHO WINS (shared `wins_over` helper) rather than a depth inequality — the answer is
  unchanged, only the mechanism moved.

Still the same guest state (0x8004E0B8) and app mode (0x80078C90); the menu transition remains the
next flow boundary.
