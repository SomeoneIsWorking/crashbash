---
id: 16
title: The libmcrd card-event callback 0x8004718C was invisible to static discovery
status: resolved
symptom: Every boot aborts on [recomp-MISS 0] no recompiled fn for 0x8004718C during the first boot-frame card probe, before frame 0
state_items: S002,S003
tags: recomp-miss,seed,memcard,hle,function-discovery
created: 2026-08-29
updated: 2026-08-29
---

## Symptom

Any 301-frame product run at framework HEAD (and reproduced identically on a fresh derivation at
the previously recorded pin `ff21584d`) aborted with signal 6 during the first update of process
state `0x8004E0B8`, before frame 0:

    [recomp-MISS 0] no recompiled fn for 0x8004718C (caller ra=0x80039504, c->pc=0x8004783C)

Host chain: `Hle::dispatchBios` -> `card_hle_a0` -> `Hle::deliverEvent` -> `rec_dispatch(0x8004718C)`
-> fail-fast. The guest had opened a SwCARD/HwCARD event in `EV_MD_INTR` mode with `0x8004718C` as
the callback, and the framework's synchronous card completion delivered it.

## Root cause

`0x8004718C` is a real runtime entry — a stackless leaf starting `addiu v0,zero,1`, preceded by the
previous function's `jr ra`, its delay slot, and TWO padding nops — but nothing points at it in any
discoverable form:

- It is never stored in guest RAM as a word (the miss report's own RAM scan says so): libmcrd
  constructs it at `0x80047280` with `addiu a3, a3, 0x718C` from a `lui`-built `0x80040000` base
  when calling OpenEvent.
- No `jal`-shaped word anywhere in MAIN or the overlay blobs targets it, so the overlay-seed scan
  never saw it either.
- `is_func_entry` signal (b) requires `jr ra` at exactly w-8; the two padding nops defeat it.

The address lives only in the framework's host-side event table, which static discovery cannot see.

## Fix (two halves)

1. **Title seed** (`game/recomp_seeds.json`, `main`): `0x8004718C` with the recorded live miss and
   the guest-side construction site — the repo's documented evidence gate for growing `main`.
2. **Framework** (psxport `02430b1b`, landed independently in this window): `is_func_entry` gains
   signal (c) — a `jr ra` boundary behind a bounded run of nop padding — so entries that only ever
   run from a computed jump sit behind link padding are discovered generically.

The seed remains recorded provenance for the live boundary even with the generic fix landed.

## Companion measurement

The first regeneration at HEAD also tripped the new emitter size guard (`f296c252`): Crash Bash's
BOOT overlay legitimately emits at 51.7x of its image (19,999,266 bytes of C from 387,072 bytes,
biggest fragment `ov_boot_gen_800AEA08` at 1.1 MB). The identical output is the substrate that ran
the verified retail boot at the pre-guard pin, so this is a measured legit case, not data-as-code:
`tools/recomp_bootstrap.py` sets `PSXPORT_EMIT_MAX_RATIO=56` with the measurement in a comment, and
a genuine leak growing past it still refuses.

## Verification

Clean-pin derivation (1095 roots -> 1757 functions, version 2026-08-29.1) contains
`func_8004718C`; `recomp_bootstrap --check` PASS; 130/130 ctest; `verify_boot.py --run` PASS; exact
301-frame native run exits 0 with zero recomp misses; frame-300 present byte-identical across the
dirty-emitter and landed-emitter builds.
