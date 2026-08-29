---
id: 18
title: A first-boot recomp-MISS fires at 0x80012840 whose dispatch case exists — flaky, only outside instrumented runs
status: open
symptom: Roughly every other long headless run fail-fasts its FIRST boot with `[recomp-MISS 0] no recompiled fn for 0x80012840 (caller ra=0x80012428, a0=0x80196A38, c->pc=0x80027944)`; the in-process reboot then runs the full frame budget clean. The address IS recompiled — `case 0x00012840u` exists in `generated/shard_disp.c` — and `routing by range` cannot miss it, so the printed miss is a lie about the mechanism.
state_items: S002
tags: recomp-miss,flaky,dispatch,attract,first-boot,diagnostic-lying
created: 2026-08-29
updated: 2026-08-29
---

## Measured

- Firing runs (no debug channels): probe8 (9000f, launched immediately after `run.sh
  --prepare-only`) and probe9 (40000f) — both missed on the FIRST boot at ~f1023-2047, then the
  automatic second boot ran the whole budget with zero misses.
- Clean runs: repro 1-6 (2600f/9000f, plain), dw 1-8 (9000f, `PSXPORT_DISPWATCH=0x80012840:ra=0x80012428`,
  8 successful dispatches of exactly that addr/ra every run), fl 1-6 (9000f, `PSXPORT_DEBUG=ovload,dispatch`)
  — 14 consecutive runs with zero misses across ~120k frames.
- The flake did not reproduce under either instrumentation, so presence of the channels changes
  the timing that selects the failing path.
- `a0=0x80196A38` matches the sixth 0x80012420 dispatch boundary already seeded (`game/recomp_seeds.json`);
  `c->pc=0x80027944` sits in the CrashBash::CdFileRead override region (retail 0x80027790), while
  `ra=0x80012428` is the `jalr` at 0x80012420 — the interrupted/hybrid pc and the call site disagree,
  which suggests the dispatch is reached from an IRQ-delivered update while a CD read override owns
  the core.

## Why the printed explanation cannot be the mechanism

`rec_dispatch` (overlay_router.cpp) routes any addr with `residentText` = [0x10000,0x79000)
(guaranteed by `static_assert` in game_config.cpp) straight to `main_dispatch`, whose switch has
`case 0x00012840u`. A genuine "no recompiled fn" for 0x80012840 through that path is impossible.
So either (a) the miss reached `rec_dispatch_miss` from a caller that bypasses the router, or
(b) the miss addr is not really the failing addr. Neither is proven; the diagnostic's canned
"likely overlay code or a mid-function coroutine resume" line is wrong for this case either way.

## Next step

~~Reproduce with a discriminator that does not depend on luck~~ — the discriminator now EXISTS and is
wired into the shipping binary (psxport `82442c0e`): `rec_dispatch` records its branch decision
(MAIN/LIVE/FIXED/AMBIG/OVERRIDE/MISSDROP) into a fixed per-core ring with NO I/O — the same shape as a
plain run, which is the whole design constraint — and `rec_dispatch_miss` rings a MISS marker and
dumps the ring oldest-first before aborting. The last decision before the MISS marker names the
branch that produced the miss; no entry for 0x80012840 at all names a bypass caller. Unit-tested
(order, wrap, silence-while-recording) in `tests/test_dispatch_decision_ring.cpp`.

Do not "fix" by re-seeding 0x80012840 — it is already seeded and compiled; that would be a bandaid on
a diagnostic that misreports its own cause.

## Status 2026-08-29 (evening)

The instrument is in place and verified, but the flake has NOT fired since: 21 consecutive clean
runs (18×9000f plain, 3×40000f concurrent), ~130k frames, zero misses of any kind. For calibration
the firing runs (probe8/9) fired roughly every other long run. The flake has therefore not yet been
observed THROUGH the ring, and two hypotheses are open:

- the ring's one store per dispatch (like every previous instrumentation) shifts the timing that
  selects the failing path; or
- the firing regime was contention-shaped — probe8/9 ran while another session was building/testing
  on the same machine, and the miss's `c->pc=0x80027944` sits in the CdFileRead override region, an
  IRQ-timing race — and that regime has not recurred.

Next session: keep looping plain runs (the ring costs one store; the dump fires only on the fatal
path), including under synthetic machine load. When it fires, the dispdec dump decides between (a)
router branch mis-selection, (b) bypass caller, and (c) a genuinely different addr reaching
rec_dispatch_miss than the diagnostic printed. Also recorded this session: the 0x800B32B4 nested
slot is a 3-module carousel — LBAs 28178 (MENU, 16 sectors), 28241 (31), 28272 (38) — confirmed by
`PSXPORT_DEBUG=crashbash-cd` over 9000-frame runs; with all three provisioned the attract demo
cycle runs clean, and the app mode still never leaves the BOOT loop (0x80078C90 / 0x8004E0B8) at
40000 frames.
