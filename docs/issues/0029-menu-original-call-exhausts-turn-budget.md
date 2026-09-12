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

Ground the next transition from the real guest call and host-turn trace. Determine whether the
call should cooperatively yield/resume, complete within a properly measured turn, or is stuck on a
missing service. Do not increase a constant budget, ignore `BudgetExhausted`, or fast-forward a
guest state to make the observer return.
