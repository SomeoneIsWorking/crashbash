---
id: 4
title: Crash Bash boot stops in the CD VSync hardware path
status: resolved
symptom: After guest main and the IRQ callback run without a recomp miss, CD sync repeats timeout and unclaimed-IRQ diagnostics until the watchdog reports stuck
tags: boot,cdrom,irq,harness
created: 2026-08-21
updated: 2026-08-21
---

## Root cause

The shared HLE lacked the BIOS `HookEntryInt`/`ReturnFromException` delivery path. The VBlank SysEnq
verifier at `0x8003B1BC` correctly declined pending CD bit 2, but the framework then had no way to
restore Crash Bash's saved `jmp_buf` and enter its master hardware dispatcher.

## Resolution

The shared path now restores the saved context, resumes `0x80031AE8` with `v0=1`, dispatches
`0x80031B58`, and reaches IRQ2 callback `0x8003F5F0`. The independent Beetle oracle confirms the same
resume/dispatcher prefix. The port verifier forbids the old CD timeout, so weakening the watchdog or
fabricating `CdInit` remains unnecessary.
