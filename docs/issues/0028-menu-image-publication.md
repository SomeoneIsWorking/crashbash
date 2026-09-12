---
id: 28
title: Direct Crash Bash player reaches MENU without authenticated image publication
status: resolved
symptom: BOOT continues after the 16-sector MENU read, then strict Lightrec dispatch refuses MENU entry 0x800B5244
state_items: S002,S003,S015
tags: menu,dynarec,loaded-image,authentication
created: 2026-09-12
updated: 2026-09-12
---

## Evidence and required transition

On pinned psxport `8b210329`, the direct player authenticated BOOT generation 3 and reached native
frames 0 and 1. The 16-sector MENU read into `0x800B32B4` partially replaced BOOT; image-range
subtraction preserved the still-executing BOOT code through the previous fault PC `0x80093244`.
After 6,390 guest cycles the next strict dispatch refused MENU entry `0x800B5244` because no
authenticated MENU generation covered it. Issue 0027 records the completed BOOT transition.

## Implemented boundary

The tracked MENU manifest identifies the exact 16-sector read at LBA 28178 into `0x800B32B4`,
the 32,768-byte SHA-256 `59c39295a4f2f50a2a472c06bc662155459052aa3934cedcc60345c267dc7e6f`,
and the entry pointer at payload offset `0x6270` containing `0x800B5244`. The completed CD read
now authenticates those bytes in guest RAM before activating and binding a MENU generation. BOOT
and MENU use one title-owned authentication policy; each sector's mapped write first retires only
overwritten image coverage, preserving unaffected BOOT fragments and native keys.

The focused Clang test used the locally provisioned USA `BOOT.BIN` and `MENU.BIN`: 2/2 MENU cases,
35 checks, including wrong tuple with valid bytes, wrong entry with the correct digest, corrupt
reload refusal, fresh-generation rebinding, and preserved BOOT PC/native key. The BOOT regression
passed 2/2 cases and 45 checks. The canonical combined Clang gate passed 28/28 title tests,
the psxport pin check, and the shipping execution-boundary check against psxport `8b210329`.

In one bounded headless retail run, BOOT generation 3 remained active, the exact 16-sector MENU
read published generation 4, and the MENU entry observer fired at `0x800B5244` with return address
`0x8001E7C0`. This crosses the previous strict-dispatch refusal without relaxing image identity.
The run next stopped inside the MENU entry's original call: `BudgetExhausted` at `0x80018AA0`
after 564,484 cycles, followed by an abort. Because the process did not shut down normally, it
did not print translated/fallback denominators; this is a reached publication milestone, not a
gameplay or fallback-parity claim. Issue 0029 owns that distinct next boundary.
