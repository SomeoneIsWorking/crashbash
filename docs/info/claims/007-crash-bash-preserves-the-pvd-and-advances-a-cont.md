---
id: C007
kind: claim
status: holds
created: 2026-08-21
tags: boot,cdrom,cdc
depends: psxport.pin, tools/verify_boot.py#judge
reconfirmed: 2026-08-21
verified_at: 2026-08-21 14:13:11
---

## Claim

Crash Bash preserves the PVD and advances a continuous whole-sector ReadN past its first payload

## Evidence

On pinned psxport 692b9b20, live PVD DMA at LBA16 retained cursor 12 and began with type=1/CD001;
the game found CRASHBSH.DAT. On landed and pinned psxport `3418a79b`, continuous mode-0xA0 sectors
advance exactly 225,792 guest cycles apart through the complete 189-sector interval, print
`done loading`, and only then reach the expected unloaded entry 0x80092BDC.

## What would falsify it

A repinned live run loses CD001, reports Cant find CRASHBSH.DAT, fails to load LBA35800 after the
LBA35799 payload, or reaches an unexpected miss before file completion.

## Re-confirmed 2026-08-21

Post-landing live run on psxport 692b9b20 preserved type-1/CD001 PVD bytes, found CRASHBSH.DAT, and advanced split-DMA continuous ReadN from LBA35799 to LBA35800.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b preserves CD001, advances mode-0xA0 continuous sectors through the 189-sector file at a 225,792-tick period, prints done loading without a same-range retry, then reaches 0x80092BDC.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b live run found CRASHBSH.DAT, scheduled every mode-0xA0 sector event 225792 guest-instruction ticks apart, sourced all 378 file DMAs entirely from the FIFO with zero depletion, completed the 189-sector read once, printed done loading, and reached 0x80092BDC.

## Re-confirmed 2026-08-21

Pinned psxport 3418a79b direct live run preserved PVD split reads, sourced all 378 file DMAs from FIFO with zero depletion, scheduled all events 225792 ticks apart, loaded once, and reached 0x80092BDC.
