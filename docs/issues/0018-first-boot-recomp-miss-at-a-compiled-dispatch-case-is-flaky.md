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

Reproduce with a discriminator that does not depend on luck: a debug channel that logs EVERY
`rec_dispatch` entry with addr/ra (not just filtered dispwatch) plus the router's per-branch
decision for addr 0x80012840, then loop plain runs until the miss fires. When it does, the log
must show which router branch (or bypass) produced the miss. Do not "fix" by re-seeding
0x80012840 — it is already seeded and compiled; that would be a bandaid on a diagnostic that
misreports its own cause.
