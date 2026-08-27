---
id: 12
title: Guest VSync timeouts remain under the retail Crash Bash frame loop
status: investigating
symptom: The MENU-reaching product trace prints seven `VSync: timeout` lines and presents no game frame
state_items: S002,S007
tags: boot,frame-loop,vsync,native-ownership,timing,presentation
created: 2026-08-27
updated: 2026-08-27
---

## Root cause

Boot dispatched whole guest main `0x8002718C`. Its application main `0x80010158` enters the
non-returning process runner `0x800270F0`, whose present callback reaches display owner `0x800272AC`.
That owner calls VSync twice, and its nested frame-arena service `0x80010488` calls VSync three more
times. The old boot verifier included `VSync: timeout` in its positive fixture, so a MENU marker was
mistaken for a clean product boundary.

The verified resident image and BOOT payload contain 51 retail VSync call sites across 22 containing
roots. The working tree owns the first display/allocator owner's five sites, three synchronous
GPU timeout/transfer sites, the boot-reached memory-card startup site, six libcd
readiness/command/sync sites, and six disc/license startup sites, leaving 30.
`docs/findings/vsync-owner-map.md` records the opcode-backed
inventory and explains why the earlier generated-source count was wrong. The exact fatal platform
trap covers every mode and every remaining caller, so a later path cannot silently regain
guest-owned time; the next reached trap identifies the next top-down owner to migrate.

## Current candidate

The working tree splits the finite boot prefix from the lifetime process runner, creates a title
FrameDriver through the shared runtime seam, ports display owner `0x800272AC` and its allocator
without guest VSync, ports the reachable synchronous GPU timeout/transfer roots, and installs the
exact fatal trap at typed entry `0x800320EC`. All replaced generated bodies remain available for A/B
comparison. The driver owns per-frame input,
measured Crash Bash VBlank callback delivery, per-field SPU output, and exactly one presentation
fence per step. No guessed sleep or synthetic guest VSync result was added.

The first bounded post-change product run used the verified USA executable and exited through the
fatal trap at `0x800486DC` with `a0=0`, `ra=0x80048700`. Its live ancestry was
`0x80027F00 -> 0x8002C894 -> 0x8003ABAC -> 0x800486DC`, before the native frame loop began. Ghidra
and the emitted retail body show this leaf is memory-card BIOS/vector startup: the old `VSync(0)` sat
between disabling automatic pad clearing and entering the setup critical section. Native boot is
single-threaded at that boundary, so `memory_card_startup.cpp` now preserves all surrounding retail
work without dispatching an ownerless guest frame wait. The next bounded product run validated that
owner by progressing beyond it and then exited through the fatal trap at `VSync(-1)`,
`ra=0x8003E6EC`, during `CD_init`. Its ancestry was
`0x80012E90 -> 0x800279A4 -> 0x80034AFC -> 0x80034B8C -> 0x8003F29C -> 0x8003EBF8 -> 0x8003E6B0`.
Those libcd bodies use four VSync queries only as timeout clocks around a controller/IRQ model that
does not exist in the port. The rebuilt candidate now owns the measured controller-ready handshake
and routes command, sync, and ISO lookup to psxport's synchronous native CD implementation. Its next
serialized run had zero VSync violations/timeouts, then timed out in the `GetTN` readiness path
`0x800349AC -> 0x8003584C` because the generic no-controller command returns no invented status
packet. The current candidate owns `0x800349AC` top-down: it reports ready only after `disc_open`
parses a non-empty CHD TOC, retaining the generated body for comparison. The next run validated that
owner, opened the CHD, and reached `load file start`, then trapped at `VSync(-1)`,
`ra=0x80034858`, through `0x800134FC -> 0x80027790 -> 0x8003470C`. The current candidate owns the
higher file-read contract at `0x80027790`: it copies every requested descriptor-relative 2048-byte
sector from the real CHD before returning and clears the guest active flag. Its generated async body
is retained, and nested residual calls remain fatal if another owner reaches them. The strict product
gate now passes that path: both loads complete and the measured MENU boundary is reached with no
guest-VSync violation or timeout.

The distinct direct 120-frame run progressed beyond MENU and exited itself with code 139 at the
fatal trap from `ra=0x8002D9E4`. Its live ancestry reaches resident `0x8002D4F4` from BOOT
`0x8008E5BC`; both captured PRESENT images were entirely black. This root is a 20-state disc/license
startup sequence. Six states query `VSync(-1)` only for controller delays. Visual inspection proved
that state 16 calls the red copy-protection failure renderer `0x8002E0F0`, which exits through BIOS
`B0:38`; the authentic-disc path completes Pause in state 18 and returns to idle state 0. The
corrected candidate owns the whole root, binds the runtime disc to the measured SCUS-94570 layout,
and records state 0. The generated root remains its A/B super and is never entered by the shipping
owner.

Static evidence after the CD candidate is green: the verified executable's process table and 21
migrated VSync JALs agree with the typed declarations, controlled ownership checks pass 7/7, the strict MENU
judge passes 13/13 and
rejects the old trace's seven timeouts, Clang 22.1.8 links the product, and the focused C++
format/tidy/size policy passes. The strict MENU boundary now passes on a real product run.

The corrected authentic-disc owner was then falsified in a direct 120-frame run. Exact child PID
`2693011` accepted the measured SCUS-94570 disc and never entered the prior copy-protection renderer
or BIOS exit path, but exited 134 through the framework watchdog before another presentation. The
live ancestry was `0x80010394 -> 0x8002D274 -> 0x8002C97C -> 0x8003A554 -> 0x800476EC`. Ghidra proves
that `0x8003A554` is stock libmcrd directory enumeration and `0x800476EC` waits for one of four
HwCARD callbacks. The callback at `0x800471DC` sets `0x80078810 = 1`, but psxport's synchronous
`B0:0x42 firstfile` / `B0:0x43 nextfile` owners preserved the empty result without delivering their
completion event. The shared candidate now delivers the existing card completion exactly once per
valid enumeration request while preserving `DIRENTRY`-or-zero. Its shipping-path Clang regression
passes; a presented non-black game frame remains unreached until the real product rerun falsifies
this boundary.

## Memory-card boundary: resolved

The rerun with the shared enumeration fix did NOT clear the trap, which falsified the diagnosis
above. Tracing with `PSXPORT_DEBUG=card,ev` showed the eight SwCARD/HwCARD `OpenEvent` calls and then
no card operation whatsoever: `firstfile` was never reached, so its completion event could not have
been the reason `0x800476EC` was entered. The real root is that Crash Bash never called
`card_overrides_init`, so the `"bu"` BIOS device was never published in the kernel device table and
libmcrd's own device walk at `0x8004799C` returned its ambiguous 0. See issue 0013 for the full
derivation and the three-way A/B; both that wiring and the shared completion event are necessary and
neither alone is sufficient.

With both in place the direct 120-frame run exits 0, completes 120/120 frames, and returns from the
native crt0 with no watchdog stall, fatal trap, recompilation miss, or guest `VSync: timeout`. The
strict serialized MENU gate passes on the same build (`--selftest` 13/13, `--run` PASS).

Static VSync ownership is unchanged at 21 of 51 sites with 30 guarded residuals; the exact fatal trap
still covers every remaining caller, and no further guest VSync root was reached in this run.

## Remaining gap

Presentation is reached but every captured PRESENT is 0% non-black, so no real game frame has been
produced yet. That is the native-graphics gap (S004, issue 0011), not a VSync-ownership defect. This
issue stays open only for that final clause of its gate.

## Next owner: the object-update method 0x8008BB48

Once the nested-module routing defect (issue 0014) was fixed, the 600-frame run advanced and
fail-fasted on a NEW first-reached guest VSync site:
`GUEST VSYNC VIOLATION: reached 0x800320EC a0=1 ra=0x8008BB88`, chain
`ov_boot 0x8007976C -> ov_boot 0x8008BB48`.

Retail `0x800320EC` decompiles to a single primitive with two non-waiting query modes and one wait
path. For `a0 == 1` it returns `(*DAT_80068BC0 - DAT_80068BC4) & 0xFFFF` — the root-counter delta
since the last sync, a SUB-FRAME timer — and skips the wait entirely; for `a0 < 0` it returns the
vblank counter `0x8006D8DC`. `0x8008BB48` calls `VSync(1)` twice: once on entry, whose value becomes
the animation time base passed to `FUN_80020344`, and once near the end with the result discarded.

Implementing `VSync(1)` natively is NOT the fix even though it does not wait. The framework trap is
explicit that nothing may reach libetc VSync "not to wait for a vblank and not to query the counter",
every one of the seven sibling ports traps it and none implements it, and this project's standing
directive is never to return a guest VSync clock.

The owner is therefore `0x8008BB48` itself, and it is a real piece of work rather than a leaf: about
496 lines of generated C, and it is reached as a VIRTUAL METHOD — `0x8007976C` walks an object list
and calls slot `[0x13]` of each object's table — so sibling methods reached the same way are likely to
need the same treatment. Size and shape are recorded here so the port is scoped before it is started.

## Resolution gate

Run the serialized product boundary with the shared memory-card enumeration fix. It must progress
beyond `0x800476EC`, reach the existing measured MENU entry/caller with no guest-VSync violation or
`VSync: timeout`, then demonstrate an actual presentation commit. Compare the first native-owned
frames against independent retail behavior before claiming timing or picture parity. Any remaining
guest VSync trap names the next top-down owner to port; do not weaken the trap or return a fabricated
counter.
