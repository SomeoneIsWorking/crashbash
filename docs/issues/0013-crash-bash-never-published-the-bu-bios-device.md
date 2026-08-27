---
id: 13
title: Crash Bash never published the "bu" BIOS device, so libmcrd waited on an operation it never started
status: resolved
symptom: The port watchdogged in the libmcrd HwCARD completion wait at 0x800476EC with no card operation ever attempted
state_items: S002,S007
tags: boot,memory-card,libmcrd,bios-device,dcb,native-ownership
created: 2026-08-27
updated: 2026-08-27
---

## Root cause

`card_overrides_init()` is the framework entry that both opens the host card image AND publishes the
card as a BIOS device (`Hle::deviceAdd("bu")`, which is the sole writer of the kernel device-table
words at `0x00000150`/`0x00000154`). Spider-Man and Tomba!2 call it from their title runtimes.
**Crash Bash never called it.** `CrashBashRuntime::registerOverrides` did not take its `Game &`.

Crash Bash's stock libmcrd does not reach the card through a BIOS vector for path resolution. Its
directory enumeration `0x8003A554` calls `0x8004799C`, and the generated body of that function is a
device-table walk, not a card operation:

- it copies the path prefix out of `"bu00:*"` into a scratch buffer, stopping at the first byte
  `< 59`, so the compared name is exactly `"bu"`;
- it reads the table base from `0x150` and the byte length from `0x154`, divides that length by the
  80-byte DCB stride (the `0xCCCCCCCD >> 38` magic division in the emitted code), and walks the array
  comparing `*(char**)(dcb + 0)` against `"bu"`;
- **on no match it returns 0 without ever calling BIOS `B0:0x42`.**

Zero is also this API's "request accepted, wait for completion" answer, so the caller could not
distinguish "no such device" from "started". `0x8003A554` therefore fell into `0x800476EC`, whose
emitted body spins until any of the four HwCARD flags `0x80078810/14/18/1c` becomes non-zero
(weighted 1/2/4/8 so the result encodes which fired). Nothing had been started, so nothing ever set
one, and the framework watchdog fired.

The device table was never published at all, so `0x150`/`0x154` held zero: the walk performed zero
iterations rather than crashing, which is why the failure presented as a silent hang instead of a
bad dereference.

## Why the earlier reading was wrong

Issue 0012 and psxport issue 0036 diagnosed this as "firstfile reports an empty card and omits its
completion event". Instrumenting the run with `PSXPORT_DEBUG=card,ev` falsified that premise
directly: the eight SwCARD/HwCARD `OpenEvent` calls appear, and then **no card operation is logged at
all** before the hang. `firstfile` was never reached. The completion event was a real defect, but it
was not the reason this trap was entered.

## Fix

`CrashBashRuntime::registerOverrides` now takes its `Game &` and calls `card_overrides_init(&game)`
before the title's own override registrations.

## Evidence

Three-way A/B on the real USA disc, 120 native frames, `PSXPORT_NATIVE_FRAMES=120`:

| build | `firstfile` reached | outcome |
|---|---|---|
| no `card_overrides_init`, with `deliverComplete` | no | exit 134, watchdog at `0x800476EC` |
| with `card_overrides_init`, no `deliverComplete` | yes, returns 0 | exit 134, watchdog at `0x800476EC` |
| with both | yes, returns 0 | **exit 0, 120/120 frames, clean return** |

Both fixes are necessary and neither alone is sufficient; the discriminator was run in both
directions rather than reasoned about. The passing run logs
`[hle] BIOS device 'bu' installed: DCB 0x8000F900 ... 80 bytes at kernel 0x150/0x154`, then
`[card] dir scan '*' -> no (further) match; 15 directory block(s) examined` and
`[card] firstfile 'bu00:*' dirent=0x801FFEF0 -> 0x00000000`, and contains no watchdog stall, fatal
trap, recompilation miss, or guest `VSync: timeout`. `tools/verify_boot.py --selftest` passes 13/13
and `--run` passes the strict serialized MENU boundary against this build.

Logs: `scratch/logs/crashbash-card-ev-trace.log` (falsifying trace),
`scratch/logs/crashbash-ab-no-complete.log` (A/B negative),
`scratch/logs/crashbash-card-device-120.log` (passing 120-frame run).

## Residual

The 120-frame run presents, but every captured PRESENT is still 0% non-black. That is the separate
native-graphics gap (S004, issue 0011), not this defect.
