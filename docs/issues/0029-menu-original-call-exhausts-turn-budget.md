---
id: 29
title: Crash Bash MENU entry original call exhausts current-turn budget
status: investigating
symptom: After authenticated MENU entry, strict original-call execution exits BudgetExhausted at 0x80018AA0
state_items: S002,S003,S015
tags: menu,dynarec,budget,host-turn
created: 2026-09-12
updated: 2026-09-12
---

## Reached boundary

The direct player authenticates MENU generation 4 from the exact 16-sector LBA 28178 read and
enters the native MENU observer at `0x800B5244` (RA `0x8001E7C0`). Its scoped call to the original
guest body then completes two other native reads, 128 sectors at LBA 17558 to `0x8018C7C0` and
129 sectors at LBA 7135 to `0x80185278`. Execution stops as `BudgetExhausted` at `0x80018AA0`
after 564,484 cycles; `callOriginal` correctly refuses an incomplete guest return and aborts.
The shutdown telemetry is absent because of the abort, so translated/fallback denominators are
still unproven. The headless direct-binary run also lacked `PSXPORT_ASSET_DIR`, leaving the host
overlay unavailable; that did not prevent reaching MENU, but visual output is unqualified.

The symbolized stack places the failed original call in `game/diagnostics/menu_boundary.cpp`'s
MENU entry observer. The shared `ExecutionBudget::currentTurn` currently returns 564,480 cycles
(`33,868,800 / 60`), and the observed 564,484 count is at that finite boundary plus the title's
call-site instruction accounting. This establishes budget exhaustion, not whether the original
body is legitimately longer than one field, awaits an undelivered event, or has diverged.

Static disassembly of the authenticated resident executable places `0x80018AA0` at `lhu r4, 0(r6)`
inside function `0x800189F4`. Its inner loop swaps the low and upper five-bit channels of each
16-bit pixel, writes through `r7`, advances both pointers by two bytes, and decrements `r5`;
the outer loop decrements `r16`. Width and height inputs are clipped against signed halfwords at
`r18+12` and `r18+14`.

A bounded rerun with one title-owned register snapshot at the existing original-call fail-fast
boundary reproduced the same exit. At `0x80018AA0`, `r5=127` inner pixels remain, `r16=211`
outer rows remain, and `r17=512` is the clipped row width. Source `r6=0x801BC18A`, destination
`r7=0x80150560`, and header `r18=0x80185278` are all mapped main RAM. The loop has advanced into
the image conversion and has finite positive counters; the source pointer remains inside the
preceding 129-sector read range `0x80185278..0x801C5A78`. This supports a legitimate one-time MENU
image transform exceeding the fixed one-field original-call budget, although a single sampled
state does not prove eventual return. A synthetic Lightrec test at the same title `GuestExecution`
boundary passes both cases: a finite decrement loop reports `BudgetExhausted` with a live nonzero
counter under a short budget, and a smaller input reaches `GuestReturn` with zero counter. The
focused Clang test passed 4/4 cases and 33 checks, with 20 translated blocks and zero fallback in
the new test case. The retail abort still prevents a whole-run fallback ledger.

The next implementation must preserve the suspended original call's guest PC, register/stack state,
image generation, native-suppression scope, and host-turn ownership across a bounded continuation,
or replace this particular conversion with a fully recovered title-native operation validated
against the retail body. This touches the shared executor/native-call contract, so coordinate its
owner before changing it. Do not increase a constant budget, ignore `BudgetExhausted`, or
fast-forward a guest state to make the observer return.
