---
id: 6
title: Crash Bash continuous ReadN stops after the first 2048-byte payload
status: resolved
symptom: After CRASHBSH.DAT is found and load file start prints, the port DMA-reads only LBA 35799 then spins forever in file-read sync
tags: boot,cdrom,cdc,dma,iso9660
created: 2026-08-21
updated: 2026-08-21
---

## Root cause

The shared synchronous controller advances a continuous `ReadN` only when its 2,340-byte whole-sector
FIFO is fully consumed. Crash Bash DMA-reads three header/subheader words and 512 payload words, or
2,060 bytes total, then waits for the next sector-ready interrupt. The remaining 280 bytes keep
`data_rd < data_n`, so `sector_consumed()` never presents LBA 35800 or raises the next INT1.

## What was tried / dead ends

This is not a game VSync implementation gap: the sampled stack is in `VSync(-1)` because
`CdReadSync` uses it as a counter while polling the read state. The decisive trace is one ReadN at
LBA 35799, DMA3 + DMA512, no later BFRD/DMA/INT1, and a zero-valued timer register at `0x1F801110`.
The shared controller must reproduce continuous-sector timing/progression; a game-local read override
or fabricated timer increment would hide the stalled CD state.

## Resolution

The shared controller now queues the next continuous-sector event after the payload DMA even though
the 280-byte tail remains. The real consumer acknowledges that INT1, writes BFRD `0 -> 0x80`, loads
LBA 35800, and performs the next header and payload DMAs. The hermetic shared test demonstrates both
answers; Crash Bash continues through LBA 35987 without a recomp miss.
