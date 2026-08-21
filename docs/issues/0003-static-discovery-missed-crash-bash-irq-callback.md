---
id: 3
title: Static discovery missed the Crash Bash IRQ callback
status: resolved
symptom: The first live resident boot stopped on recomp-MISS 0x8003B1BC after entering guest main
tags: boot,recompiler,irq
created: 2026-08-21
updated: 2026-08-21
---

## Root cause

The callback target is stored in guest RAM at 0x8006D98C and reached indirectly while guest CD sync
0x8003EDBC calls VSync 0x800320EC. Direct-call and binary table discovery therefore had no static edge
to 0x8003B1BC.

## Resolution

The observed miss supplied the target and Ghidra independently confirmed 0x8003B1BC as the IRQ
callback that tests event flags and dispatches a registered handler. It is now the sole explicit
resident seed. Emission increased from 338 roots / 906 functions to 339 / 907, and the subsequent
real boot reached the callback with no recomp miss.
