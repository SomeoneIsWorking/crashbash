---
id: 30
title: The suspend/resume consumer half is written against psxport APIs that do not exist
status: blocked
symptom: build/migration fails — "no member named 'resumeOriginal' in namespace 'psx::cpu'" and "no member named 'resumeGuestToReturn' in namespace 'psx::cpu'"
state_items: S002,S003,S015
tags: dynarec,budget,host-turn,framework,blocked
created: 2026-09-19
updated: 2026-09-19
---

## What is in the worktree

Uncommitted work answering issue 0029: rather than aborting when a MENU original call exhausts the
current-turn budget, suspend it and resume it on the next host turn. It adds
`OriginalCallMode::{RequireReturn,YieldOnBudget}`, a `SuspendedOriginal {image, key, resumePc,
returnPc}` in `GuestExecution`, `measuredGuestCallSlice`, and a new `FrameGuestCall` owner
(`game/core/frame_guest_call.{h,cpp}`) for one bounded outer process call across host turns. The
codemap is updated in the same change. The design is coherent and the ownership split is right.

## Why it cannot build

It calls two framework entry points that psxport does not provide:

- `game/core/guest_execution.cpp:192` — `psx::cpu::resumeOriginal(core, key, resumePc, returnPc, budget)`
- `game/core/frame_guest_call.cpp:63` — `psx::cpu::resumeGuestToReturn(core, resumePc, returnPc, budget)`

Neither has ever existed. `git log -S` over all of psxport's history for both names returns nothing,
so this is not drift from a removed or renamed API — the consumer half was written first and the
framework half was never written. Measured 2026-09-19 against psxport `10071776` with Clang: the
build stops with these two errors (4 diagnostics) at `[107/172]`; everything before it compiles.

## Next step

Decide the framework contract before writing more consumer code, because the suspended state belongs
to whoever owns the executor:

- `resumeOriginal` must re-enter a specific authenticated image generation with its override still
  suppressed, from a saved guest PC, and return a typed `ExecutionResult` — so psxport has to hold
  the suppression across the host-turn boundary, not just for the duration of one call.
- `resumeGuestToReturn` is the ordinary case: continue translated execution from a saved PC until a
  given return address or the budget runs out.

Implement both in psxport with their own tests (including the negative: a resume whose image
generation has been retired must fault, which the consumer already expects at
`guest_execution.cpp:185`), land and push psxport, bump `psxport.pin`, then build this consumer
against the pinned revision. Do not stub the two functions locally in this repository to make the
build pass — that would fork the executor.

Crash Bash is behind Tomba! 2 and Spyro in the single-title order, so this stays blocked until those
titles' gates are complete.
