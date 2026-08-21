---
id: 4
title: Crash Bash boot stops in the CD VSync hardware path
status: open
symptom: After guest main and the IRQ callback run without a recomp miss, CD sync repeats timeout and unclaimed-IRQ diagnostics until the watchdog reports stuck
tags: boot,cdrom,irq,harness
created: 2026-08-21
updated: 2026-08-21
---

## Root cause

The resident code and measured IRQ callback are present, but the current port has no measured native
ownership for the CD/VSync hardware interaction that should advance this boot wait. The watchdog is
reporting the real lack of progress; disabling it or special-casing the wait would only hide the
missing hardware service.

## Next investigation

Capture the true oracle at the same boundary, measure the CD/VSync register, IRQ registration, and
callback sequence, then implement the smallest faithful psxport/game ownership boundary supported by
that evidence. Do not guess an IRQ seed/address or weaken the timeout.
