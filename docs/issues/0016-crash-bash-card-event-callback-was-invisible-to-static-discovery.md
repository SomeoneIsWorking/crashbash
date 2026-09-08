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

## Runtime contract

The static discovery implementation and its seed metadata have been removed. Preserve the measured
callback address and its construction site as binary evidence: the shared HLE event delivery must
dispatch this callback through the current authenticated resident image, discovering and translating
the cold block on demand. No offline root list is required. The former boot observation establishes
the callback's role, not conformance of the replacement runtime path.
