---
id: 5
title: Crash Bash ISO lookup reads the PVD payload from LBA 17
status: resolved
symptom: After CD IRQ delivery works, boot repeatedly prints Cant find CRASHBSH.DAT and the watchdog stops in the CdSearchFile PVD read path
tags: boot,cdrom,cdc,iso9660
created: 2026-08-21
updated: 2026-08-21
---

## Root cause

The shared CD controller treats every asserted BFRD write after any FIFO consumption as a request for the next sector. Crash Bash asserts BFRD, DMA-reads three header/subheader words from whole-sector LBA 16, writes BFRD=0x80 again while it is still asserted, then DMA-reads 512 payload words. The second write incorrectly increments to LBA 17 and reloads the FIFO, so the PVD type/CD001 fields land at the wrong offsets and FUN_80034F64 rejects the disc.

## Measured contract

GDB at cdc_write captured `00 rd=0/2340`, `80 rd=0`, DMA3 -> `rd=12`, repeated `80 rd=12`, DMA512, then `00`. A correct shared BFRD latch keeps the repeated assertion on the current FIFO; a hermetic split-DMA test must fail if the LBA advances or the returned bytes do not equal raw LBA16 offsets 12..2060. No game-local CdSearchFile override is acceptable.

## Resolution

The shared controller now latches BFRD and preserves the FIFO cursor on a repeated asserted write.
The real consumer reaches the 512-word PVD payload DMA at LBA 16 with `data_rd=12`, `data_n=2340`,
and bytes `01 43 44 30 30 31 01 00` (`type=1`, `CD001`). `Cant find CRASHBSH.DAT` is absent and the
game prints `load file start`.
