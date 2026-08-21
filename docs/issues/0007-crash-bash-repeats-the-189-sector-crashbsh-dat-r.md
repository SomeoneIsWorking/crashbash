---
id: 7
title: Crash Bash repeats the 189-sector CRASHBSH.DAT read instead of completing
status: investigating
symptom: After continuous ReadN advances through LBA 35799..35987, the game restarts Setloc at LBA 35799 and never prints done loading
tags: boot,cdrom,libcd,dma,watchdog
created: 2026-08-21
updated: 2026-08-21
---

## Root cause

Not established yet. The continuous controller is no longer stalled: each pass reads LBA
35799..35987 with 189 matching header and payload DMAs, then Pause/Setloc/ReadN restarts the same
range. The read-completion result or the higher-level loader state is therefore the next boundary.

## What was tried / dead ends

The watchdog backtrace alone is misleading here because it fired while the process was decompressing
a CHD hunk for an actively advancing sector stream. Disabling the watchdog would hide both the
legitimate progress and the repeated-read bug. No game-local file-read override is warranted before
the guest result/state transition is measured.

## Next investigation

Capture the guest return/result state at `0x80027790`, `0x80027944`, and the `0x800348A8` read-sync
completion after LBA 35987, then compare the same boundary with the true oracle. Separately, route
verified CD/FMV progress into the shared watchdog owner rather than weakening its timeout.
