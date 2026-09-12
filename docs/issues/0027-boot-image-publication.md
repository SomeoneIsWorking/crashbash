---
id: 27
title: Direct Crash Bash player reaches BOOT entry without authenticated image publication
status: investigating
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
loaded at `0x80078C90`. `cd_file_read.cpp` copies the requested sectors into guest RAM but never
authenticates or activates the completed code image in psxport's `ImageCatalog`, and the title's
`GuestExecution` has no BOOT binding. Refusing the dispatch is the correct wrong-image guard.

## Next owner

At the completed module-load boundary, authenticate the bytes actually written to guest RAM against
the tracked loaded-module identity, activate the corresponding physical range and generation, and
bind its pending native owners before any entry can run. Replacement of the nested MENU/DAT slot must
retire stale keys and invalidate translated code before reuse. Use the existing module manifests as
the identity source; do not infer an image solely from a guest address or weaken the dispatcher.
