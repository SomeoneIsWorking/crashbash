---
id: 17
title: The attract flow advances through runtime-patched computed dispatch boundaries, and the authored key ord carrier must be uniform in the key
status: open
symptom: Raising the frame cap past 301 fatal-aborted the render (OT key->ord non-injective), then fail-fasted on recomp-MISS 0x80016F34 and 0x8001DFF8; a 1200-frame probe now completes exit 0 and the attract sequence visibly advances past the EUROCOM logo
state_items: S002, S004
tags: attract,flow,dispatch,seeds,keyord,ot,authored-order,d32
created: 2026-08-29
updated: 2026-08-30
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
`pointer_table_funcs` cannot see them. They are nevertheless ordinary stripped-binary function
boundaries immediately following a previous `jr ra`. The shared emitter now applies its existing
mergeable return-boundary discovery to MAIN as well as overlays, so these entries are derived from
the retail layout instead of being accumulated as title seeds:

- `0x80012678`
- `0x80016F34` — reached at ~frame 300+, a0=0x801D4B58
- `0x8001DFF8` — reached after the key->ord fix, a0=0x801DAB70
- `0x80012840` — later member reached with a0=0x80196A38

All four explicit seeds were deleted. A fifth redundant MAIN seed, callback `0x8004718C`, is already
derived by the constructed-pointer scan. Re-emission with only residual seed `0x8003B1BC` preserves
all five dispatch cases and the exact 2,450-function generated output. Future family members matching
the same retail boundary no longer require miss/add/rebuild work; a miss without generic file evidence
remains a measured residual rather than weakening native fail-fast dispatch.

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

## The uncontrolled attract carousel exercises three members of the nested-slot family

A `PSXPORT_DEBUG=crashbash-cd` census over two boots x 9000 frames shows the 0x800B32B4 slot is a
carousel, not a one-shot nest — it receives **LBA 28178 (16 sectors, = MENU), LBA 28241 (31 sectors),
and LBA 28272 (38 sectors)**, repeatedly, in the order 28178 -> 28272 -> 28178 -> 28241 as the attract
cycle changes scenes. DAT28241 (uncommitted work found in the tree this session, now landed) covers
the third member: its image offset 0xD8E0 is the earlier miss address 0x800C0B94, and its name table
is animation states (BREATHE/BANK/IDLE_A/TAUN...) like DAT28272's — same family, another attract
demo scene. With all three stems provisioned:

- census run: 2 boots x 9000 frames, exit 0, ZERO recomp-MISS (`scratch/logs/cd-census4.log`).
- post-rebuild verify: 2400 frames, exit 0, zero MISS/FATAL (`scratch/logs/verify-28241.log`), on the
  pin commit 625f8e69 with psxport 23/23 + crashbash 23/23 + python suite green.

Before DAT28241 was in the build, the miss address VARIED per boot (0x800C3434, 0x800C0B94) with the
same caller (ra=0x80078D10) — which demo scene the attract cycle loads first is per-boot, so a
single-stem build fail-fasts nondeterministically. Any future miss in the nested slot must first be
correlated with the immediately preceding CD read: "module not provisioned" is the default
hypothesis, and range attribution is inherently ambiguous because every alternative shares a base.

## Two diagnostic defects observed this session (open)

1. **The dbg server endpoint dies across the in-process boot retry.** After a fail-fast miss and
   reboot, the second `PSXPORT_DEBUG_SERVER` bind fails with "Address already in use" and the port
   the first boot advertised refuses connections (`scratch/logs/start-probe1.stdout`). Driving pad
   input at the attract loop is therefore impossible in any run that ever fail-fasted.
2. **`PSXPORT_DEBUG=cd` segfaulted (exit 139) at the fail-fast/reboot boundary**
   (`scratch/logs/cd-census1.log` ends at the miss RAM dump); the game-local `crashbash-cd` channel
   runs the same shape clean. Not root-caused.

## Historical uncontrolled-flow result at 9000 frames

Even with the carousel fully provisioned, both boots dwell in state 0x8004E0B8 / app mode
0x80078C90 for all 9000 frames while the demo scenes visibly cycle (model draws oscillate
0 -> 71 -> 2 -> 3 -> 80 -> 6 across the dwell logs; the slot modules reload). The attract demo
loops on itself; the menu transition was NOT reached by time alone. Later SIO A/B and static MENU RE
proved that START was the wrong action for this type-`0x0101` state: Cross is the measured accept
input, now exercised by `tools/verify_menu_accept.py`.

### The nested slot is a four-image family; the Cross path adds DAT28136

The 9000-frame probe exposed the miss class again at `0x800C0B94` (same caller `ra=0x80078D10`,
same dispatch site as `0x800C3434`), and this time the `crashbash-cd` trace caught the load that
fed it: `31 sector(s) from LBA 28241 to 0x800B32B4` — a THIRD module in the nested slot, distinct
from both MENU (LBA 28178) and DAT28272 (LBA 28272). The nested slot is a FAMILY of alternatives
rotated by flow state, not a fixed attract-only carousel.

DAT28241 is provisioned as overlay stem `DAT28241` (`titles/crashbash/dat28241_module.json`, 6/6
facts). Byte-verified: the miss RAM dump over the slot
differs from this image in only 567/63488 bytes (the runtime-patched pointer slots) and from
DAT28272 in 54571 — the two are distinct images. The miss address (image offset 0xD8E0) decodes
as valid R3000A. Its name table is animation states too (BREATHE/BANK/IDLE_A/TAUN...), the same
family as DAT28272; provisionally another attract demo scene.

Verified at psxport `625f8e69` on the real disc: with all three nested-slot modules provisioned,
the flow runs the full attract demo cycle with ZERO recomp-MISS. The demo scenes render real
native content — measured screenshots at f~8k/13k/19k show a lightning temple interior, a
character close-up, and a complete Crash Bash bumper arena. 9000- and 40000-frame runs exit 0.

The controlled Cross path then identified the fourth nested image: `42 sector(s) from LBA 28136 to
0x800B32B4`, immediately before BOOT dispatch at `ra=0x80078D10` reached `0x800B4E1C` (+0x1B68 in
the image). Its exact retail slice is `CRASHBSH.DAT+0x0367E000`, size `0x15000`, SHA-256
`c5052413c19fcab896ffa19d18b278f4418181d6c927bc903f9cc3de9e6e43ad`. It is provisioned as entryless
stem `DAT28136`; total module verification is now 32/32 across BOOT and four nested alternatives.
Whole-image generation discovers the old miss without a manual seed. The exact-identity verifier
proves registration body `0x800B4E1C` replaces app callback `0x80093038` with `0x800B4694`, then
observes that update execute. A 2400-frame real product run exits 0 with no recomp miss, fatal, or
guest-VSync violation.
