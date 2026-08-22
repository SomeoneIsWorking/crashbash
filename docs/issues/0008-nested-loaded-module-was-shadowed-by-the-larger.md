---
id: 8
title: Nested loaded module was shadowed by the larger BOOT range
status: resolved
symptom: After MENU DMA completed, indirect dispatch at 0x800B5244 still missed because BOOT also matched the address and retained signature
tags: overlay,routing,loaded-code,framework
created: 2026-08-22
updated: 2026-08-22
---

## Root cause

The emitted BOOT range `[0x80078C90,0x800D7490)` contains the later MENU range `[0x800B32B4,0x800BB2B4)`. MENU overwrites only a nested slice, so BOOT's first-32-byte signature remains valid. The old framework resolver returned the first matching fixed range, allowing BOOT to shadow MENU.

## Evidence

The real consumer loaded both modules and missed `0x800B5244`, held at RAM `0x800B9524`, until the shared resolver selected the unique most-specific matching range. The focused framework fixture remains green when names and table order change, refuses equal-specificity ambiguity, and falls back to BOOT when MENU's signature is absent. After the fix, the consumer executes `ov_menu_gen_800B5218` and reaches the later resident CD state machine without a recomp miss.

## Resolution

The shared overlay resolver now selects the unique most-specific loaded range rather than filename/table order. Crash Bash carries measured BOOT and MENU manifests and no routing workaround.
