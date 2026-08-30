---
id: 22
title: Cross transition loads an unprovisioned DAT28136 module
status: resolved
symptom: A 1200-frame forced-Cross run loads 42 sectors from LBA 28136 into 0x800B32B4 and recomp-MISSes 0x800B4E1C from ra 0x80078D10
state_items: S002,S004
tags: flow,cross,loaded-module,recomp-miss,provisioning
created: 2026-08-30
updated: 2026-08-30
---

The shipping CD path reports a completed 42-sector read from disc LBA 28136 to the shared nested-module base 0x800B32B4 immediately before execution reaches 0x800B4E1C. The miss is +0x1B68 inside that measured image and the active runtime slot is still attributed BOOT only because the image is absent from the module registry. Proper fix: measure the exact CRASHBSH.DAT slice, add it as an identity-verified entryless nested module, regenerate from that whole image, and rerun the same Cross flow. Do not seed the miss address or special-case dispatch.

### Resolution (2026-08-30)
Measured CRASHBSH.DAT+0x0367E000 (42 sectors, SHA-256 c5052413c19fcab896ffa19d18b278f4418181d6c927bc903f9cc3de9e6e43ad) as entryless DAT28136 at 0x800B32B4. Provisioning passes 32/32 facts; regeneration emits 1,411 roots into 2,593 functions without a miss seed. The exact-identity Cross verifier proves 0x800B4E1C replaces callback 0x80093038 with 0x800B4694 and that update executes; a 2,400-frame real product run exits 0 with no recomp miss, fatal, or guest-VSync violation.
