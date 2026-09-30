---
id: 29
title: Crash Bash MENU entry original call exhausts current-turn budget
status: resolved
symptom: After authenticated MENU entry, strict original-call execution exits BudgetExhausted at 0x80018AA0
state_items: S002,S003,S015
tags: menu,dynarec,budget,host-turn
created: 2026-09-12
updated: 2026-09-30
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

## Resolved 2026-09-30 — classified by cause, and the resume cap is derived rather than raised

**This budget IS on the path to the main menu and to a controlled mode**, so it had to be classified,
not waved through. It was, and it is a legitimate one-time finite conversion.

### The body is finite, and that is now a property of the bytes rather than of one sampled state

`FUN_800189F4` is `0x800189F4..0x80018B04`, read in full from the authenticated resident executable:

    80018A10  lh    $v1, 0xc($s2)      ; header width, signed halfword
    80018A28  lh    $v0, 0xe($s2)      ; header height, signed halfword
    80018A18  slt   $v0, $v1, $s1     ; clamp against the caller's width
    80018A30  slt   $v0, $v0, $s0     ; clamp against the caller's height
    80018A38  mult  $s1, $s0          ; pixels = width * height
    80018A6C  addiu $s0, $s0, -1      ; outer induction variable
    80018A90  addiu $a1, $s1, -1      ; inner induction variable
    80018AA0  lhu   $a0, ($a2)        ; inner body: load, swap 5-bit channels, store, advance both
    80018ACC  bne   $a1, $t0, 0x80018aa0    ; back-edge, $t0 = -1
    80018AE4  bne   $s0, $t1, 0x80018a90    ; back-edge, $t1 = -1

There are **exactly two backward branches in the whole function**, both compare a register that the
preceding block decremented against `-1`, and there is no data-dependent exit: the iteration count is
the product of two clamped halfwords, known before the first iteration. The one sampled state issue
0029 recorded (`r5=127`, `r16=211`, `r17=512`) is therefore not evidence of an unbounded loop; it is
one point on a counted descent, and the count now comes from the code.

### The cap, and where the number comes from

MEASURED on the pinned Lightrec product, MENU entry `0x800B5244`:

    [crashbash-guest] guest call 0x800B5244 to return address 0x8001E7C0 outlived one host turn:
      7 turn(s), 3441400 guest cycles (6.097 display fields)

The body is 2 bytes in and 2 bytes out per pixel over the 128-sector / 262,144-byte image that entry
converts, i.e. **131,072 pixels**, which gives the per-pixel cost this title's own load path measures:

    3,441,400 cycles / 131,072 px = 26.256 cycles/px        (one display field = 564,480 cycles)

The largest payload this title's own load path can hand the body is BOOT's, 189 sectors =
387,072 bytes = **193,536 pixels**, so a complete conversion of the largest tracked image costs

    193,536 px * 26.256 = 5,081,442 cycles = 9.00 display fields

and the shipped `kGuestCallTurnCap = 12` display fields is **6,773,760 cycles = 257,991 pixels =
a 252-sector image**: it covers the largest tracked payload with 33% headroom and still fails inside
0.2 s of wall clock on a guest loop. The number was not raised to make a symptom disappear; it is the
measured per-pixel cost multiplied by the largest image the loader can produce, rounded up, and the
run-end census prints the deepest turn count beside it so the derivation stays falsifiable from a log
(`deepest 7 turn(s) against a cap of 12`).

### Whole-run denominators on the same run that reaches the menu

257,083 guest calls completed, 6 needed a resume, 0 faults, 264,839 executor calls, 18,931,728
executed blocks, 287,182,711 executed instructions, **0 fallback blocks / 0 fallback instructions**
with every refusal reason 0. Nothing in the path is interpreted.

### The framework API the suspended state needed

`psx::cpu::resumeOriginal` and `psx::cpu::resumeGuestToReturn` now exist in the pinned framework and
`game/core/guest_execution.cpp` consumes them, so issue 0030's "the consumer half was written first and
the framework half was never written" is stale: the contract landed and this tree builds against it.
What 0030 still asks for — the negative case where a resume whose image generation has been retired
must fault — is implemented in `runGuestCallToReturn` for an original call and refused loudly rather
than resumed into.
