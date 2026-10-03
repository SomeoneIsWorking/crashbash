# 0034 — the 16:9 HUD is drawn TWICE, so the anchored copy lands beside the unanchored one

**Status:** RESOLVED — framework `6b5cee55` + this title · **Work:**
`game/render/widescreen/ui_anchor.{h,cpp}`, `game/render/widescreen/hud_layout.h`,
`game/execution/title_adapter.cpp`, `game/render/sprite_quad_capture.cpp`
· **State:** S019

## What was built

The horizontal anchoring policy exists and is verified: `game/render/widescreen/ui_anchor.h` owns
the arithmetic, `game/render/widescreen/hud_layout.h` owns the classification, and
`tests/test_ui_anchor.cpp` exercises both against the layout read off a live Crashball match. At
16:9 a left-edge element is corrected by `-margin` (−86 px at 512→684), a centred one by 0, and a
right-edge one by `+margin` (+86 px). At 4:3 every one of them is 0, so the 4:3 picture is unchanged
by arithmetic rather than by a saved copy.

## The element census, with addresses

Every 2D element reaches the native producer through three shared submit leaves
(`0x8002992C` Gouraud-textured, `0x80029D28` flat-textured, `0x8001A0D8` screen-colour), so the leaf
cannot identify the element. The identity used is the CALLER — `ra` at the override entry — read out
of a live match with the producer's per-element census, not assumed:

| `ra` | element | authored x (of 512) | class |
|---|---|---|---|
| `0x80019EE8` (`jal 0x80029D28` at `0x80019EE0`) | per-player portraits, 64 px wide, y 12..43 | 32 / 128 / 320 / 416 | side-of-centre |
| `0x8007A010` | first digit of each lives counter, 32 px wide, y 37..52 | 43 / 139 / 331 / 427 | side-of-centre |
| `0x80079F9C` | second digit of each lives counter | 62 / 158 / 350 / 446 | side-of-centre |
| `0x80024688` (`jal 0x8002992C` at `0x80024680`) | string renderer, glyph by glyph | centred blocks | centred |
| `0x80024640` (`jal 0x80029D28` at `0x80024638`) | flat sprite through the same walk | — | centred |
| `0x800245F4` (`jal 0x80029D28` at `0x800245EC`) | countdown / WINNER stack | 196..227 | centred |

The countdown is declared centred rather than classified: its authored centre (211) is 45 px LEFT of
the 512 centre, so "left of centre means left edge" would move it 86 px further left. It is the
guest's own fixed-x centred stack.

## The defect, and the three measurements that fix its cause

At 16:9 the presented HUD shows **eight** portraits and scores where the guest authors four.

1. **The anchored copy is where it was asked to be.** `PSXPORT_DEBUG=uihud`, presented frame 3600:
   `element=panel anchor=left-edge authored=(32,64) frame=512->684 margin=86 correction=-86` and
   `anchor=right-edge … correction=+86`, ~12 elements a frame. The picture agrees: the outermost
   pairs are hard against the widened edges and the inner four stayed where the framework's
   centring puts them.
2. **The unanchored copy is not the port's.** With every native sprite submission skipped
   (`{ continue; }` in `submitSpriteQuads`), the portraits and digits are STILL presented, at the
   framework's centred positions. Whatever draws it, it is not `native_sprite_quad_producer`.
3. **It tracks the frame rather than being a stale echo.** Across presented frames 2600/3000/3600
   the surviving HUD's ink runs change (`226..245` → `229..243`), so it is drawn live each frame.

So psxport also replays the guest's OWN GP0 packets, shifting each by the same margin
(`gpu_native.cpp`, `gp0_exec` → `ws_2d_local_x`), and the title's native producer draws the same
elements again. At 4:3 the two coincide and the duplication is invisible; anchoring separates them,
which is why the baseline never showed it.

## Why the title could not suppress its half, and what closed it

psxport has exactly the facility for this: `runtime/psx/guest_packet_filter.h` +
`GuestPacketOwnerScope` + `OtAttr`. Wiring it is 12 lines, and they were written and measured —
`filter.setSuppressed(kSpriteQuadSubmit, true)` for the three leaves, the owner scope around
`callOriginal`. **The capture was byte-identical with and without them.** The reason is structural:
`GuestPacketFilter::suppressesPacket` answers from recorded store spans, those come from
`OtAttr::pool_range_uncached`, and that read the packet-pool window off a legacy `GameConfig`.
Crash Bash installs a typed `GameRuntime` (`TitleAdapter final : public GameRuntime`), so
`core.cfg == nullptr`, the window is unknown, and the filter could never match. The framework's own
warning named it: *"packet-pool attribution is STRUCTURALLY BLIND here"*.

`docs/findings/crashbash-packet-pools.md` holds the six globals a legacy config would have needed
(parity 0 base/end/current `0x8005F790/0x8005F794/0x8005F798`; parity 1
`0x8006379C/0x800637A0/0x800637A4`).

**The fix, psxport `6b5cee55`:** `GameRuntime::guestPacketPoolWindows()` — the same direct-runtime
seam as `guestCdStreamCallbackLayout` / `discEnvVar` / `hostIdentity`, default null — carrying either
representation (a fixed base+stride, or the two live pointer-global pairs). `OtAttr` reads it when
`core.cfg` is null. The cache that made this unreachable rather than merely absent was fixed too: it
was keyed on `c->cfg`, which is null for every typed runtime, so it never re-read anything. The
legacy `GameConfig` path is untouched — `declaredGuestPacketPoolWindows()` refuses a Core that has a
config, which the framework test pins.

**Title-side, unchanged from what was here:** the six measured globals in
`TitleAdapter::guestPacketPoolWindows()`, and the three `setSuppressed` declarations plus the
`GuestPacketOwnerScope` around each `callOriginal`. The scope is entered only when this port actually
decoded the quad, because a call it refused submits nothing natively and must leave its packets
visible.

## The result

`PSXPORT_DEBUG=uihud` run, presented frame 2000, live four-player match, 16:9 — **four** portraits
and **four** score groups, where the pre-fix capture showed **eight**:

| element | sink x (960 px sink, kx = 960/684 = 1.4035) | native | expected |
|---|---|---|---|
| P1 portrait | 55..135 | 39..96 | authored 32, left edge |
| P2 portrait | 195..260 | 139..185 | authored 128, left edge |
| P3 portrait | 700..770 | 499..549 | authored 320 + 172, right edge |
| P4 portrait | 825..890 | 588..635 | authored 416 + 172, right edge |
| countdown | 410..530 | 292..378 | centred, unchanged |
| lives flag | 232..268 | 166..191 | centred, unchanged |

`[producers] run-end: OtAttr spans recorded 16 (overflow 0)` — the feed is live where it previously
reported 0 — and the "STRUCTURALLY BLIND" warning is gone from the run.

## The two caveats, stated rather than buried

1. **The 4:3 leg did not reach a live match in this build.** The tracked replay's route is
   host-timing dependent — it already varied run to run before any of this — and at 4:3 it now takes
   a different route than at 16:9 (three consecutive 3,620-frame runs reached the same, non-match,
   state). What 4:3 evidence there is: the correction is arithmetically 0 for every class and is
   unit-tested as such, and the 4:3 captures taken show the authored layout with centred text correct.
   A 4:3 live-match capture is still owed.
2. **4:3 is no longer byte-identical to before, and cannot be.** Suppression is aspect-independent, so
   the guest's second copy of every sprite quad is gone at 4:3 too. The two copies coincided in
   POSITION before, not in rasterisation — measured at ~34 px of differing digit ink — so removing one
   is a real rasterisation change at 4:3 even though the layout is untouched. That is the correct
   outcome (one drawer rather than two), but it is a change, and this issue does not claim otherwise.
