---
id: C007
kind: claim
status: holds
created: 2026-08-21
tags: boot,cdrom,cdc
depends: psxport.pin, tools/verify_boot.py#judge
---

## Claim

Crash Bash preserves the PVD and advances a continuous whole-sector ReadN past its first payload

## Evidence

On pinned psxport 692b9b20, live PVD DMA at LBA16 retained cursor 12 and began with type=1/CD001; the game found CRASHBSH.DAT. The final live continuous trace ACKed after LBA35799 DMA3+DMA512, wrote BFRD 0 then 0x80, loaded LBA35800, and performed both next DMAs with no Cant-find or recomp miss.

## What would falsify it

A repinned live run loses CD001, reports Cant find CRASHBSH.DAT, fails to load LBA35800 after the LBA35799 payload, or reports a recomp miss.
