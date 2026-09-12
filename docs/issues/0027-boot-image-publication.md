---
id: 27
title: Direct Crash Bash player reaches BOOT entry without authenticated image publication
status: resolved
symptom: The first native frame dispatches 0x80092BDC after loading BOOT, but image-qualified Lightrec dispatch refuses an unknown code image
state_items: S002,S003,S015
tags: boot,dynarec,loaded-image,authentication
created: 2026-09-12
updated: 2026-09-12
---

## Evidence

The direct player opens the USA CHD, completes the initial `CRASHBSH.DAT` load, enters the native
frame loop, then reports `guest address 0x80092BDC resolves to zero or multiple active code images`
after 153,924 guest cycles. The BOOT manifest names that address as the entry of a 189-sector image
loaded at `0x80078C90`; an earlier native CD trace records the exact 189-sector read from LBA 35799
to that destination. `cd_file_read.cpp` copies the requested sectors into guest RAM but never
authenticates or activates the completed code image in psxport's `ImageCatalog`, and the title's
`GuestExecution` has no BOOT binding. Refusing the dispatch is the correct wrong-image guard.

## Implemented boundary

The title's completed CD read now recognizes the full BOOT tuple from `boot_module.json` (disc LBA,
destination, and sector count), checks the SHA-256 and entry pointer of the bytes in guest RAM, then
invalidates the written range, activates its physical image generation, and binds the pending BOOT
native owners. A failed sector read or a digest mismatch cannot publish the image. At every mapped
CD sector write, `GuestExecution` subtracts overwritten physical bytes from authenticated image
coverage and retires native keys at overwritten entry addresses. It preserves the same image
generation and native keys for untouched fragments. The resident catalog covers only the
CRT0-audited pre-BSS code interval, so retirement of BOOT cannot reveal stale Resident identity in
the BSS/heap bytes that BOOT overwrote; unaffected resident functions remain callable.

Focused Clang tests exercise the shipping publication owner with empty/corrupt bytes, mapped sector
overlap, and the locally provisioned USA `BOOT.BIN` (2/2 cases, 45 checks). The title adapter's
retail executable test passed 4/4 cases and 50 checks. The canonical title verifier passed 27/27
tests against pinned psxport `8b210329`.

The first bounded direct-player run after BOOT publication did authenticate BOOT generation 3,
entered app mode at `0x80092BDC`, and reached frames 0 and 1. A 16-sector MENU read into
`0x800B32B4` then overwrote only part of BOOT. Full-generation retirement incorrectly removed
identity from still-BOOT PC `0x80093244` and caused a strict image fault after 9,240 guest cycles.
After range subtraction and pinning psxport `8b210329`, a second bounded headless retail run again
authenticated BOOT and entered frames 0 and 1. It survived the same MENU read and continued through
the prior BOOT fault PC `0x80093244`; strict dispatch then refused the newly reached MENU entry
`0x800B5244` after 6,390 guest cycles. This confirms the BOOT publication and partial-overwrite
transition locally. Authenticated MENU publication is the next distinct work point; image dispatch
must remain strict. Issue 0028 owns the newly reached MENU image publication.
