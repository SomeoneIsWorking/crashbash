---
id: C011
kind: claim
status: holds
created: 2026-08-22
tags: loaded-code,boot
depends: tools/verify_menu_accept.py#judge, game/diagnostics/menu_boundary.cpp, titles/crashbash/dat28136_module.json
reconfirmed: 2026-08-30
verified_at: 2026-08-30 04:46:17
---

## Claim

Crash Bash reproducibly provisions and executes BOOT plus every measured nested CRASHBSH.DAT code module reached by the controlled attract and Cross paths

## Evidence

Real USA DAT verification passes 32/32 module facts across BOOT and four nested alternatives. Current
recompilation emits 1,411 roots into 2,593 functions and discovers DAT28136 registration
`0x800B4E1C` without a manual seed. The exact-identity Cross differential reaches MENU, observes
DAT28136 replace app callback `0x80093038` with `0x800B4694`, and observes that update execute. Its
10/10 controls reject missing/wrong transition evidence, and a 2400-frame Clang product run exits 0
without a fatal, recomp miss, or guest-VSync violation.

## What would falsify it

If any registered USA DAT payload identity changes, generated ranges omit a registered module, the
real consumer no longer reaches the measured MENU entry, or the controlled Cross path no longer
installs and executes DAT28136 callback `0x800B4694` without a forbidden runtime failure.

## Re-confirmed 2026-08-22

Against exact pinned psxport ad5cf802, real USA provisioning passed executable 11/11 and loaded-module 14/14, recompilation emitted 1,063 roots into 1,724 functions, full Clang CTest passed 8/8, the real consumer executed ov_menu_gen_800B5218 without a recomp miss, and the CDC diagnostic proved GetTN response drain/ACK before 168357 empty polls at 0x8002DE2C.

## Re-confirmed 2026-08-22

Final default-build verification against exact pinned psxport ad5cf802: full Clang CTest passed 8/8, the real-disc boot gate passed on 132 runtime lines with BOOT and nested MENU execution and no recomp miss, and the CDC trace proved GetTN drain/ACK before 1,827,971 empty polls at resident 0x8002DE2C.

## Re-confirmed 2026-08-22

Against pinned psxport ad5cf802, the corrected real-disc verifier deterministically traced guest poll state 0x8002DE2C and passed 7/7 controlled answers; full scratch Clang CTest passed 8/8. The bootstrap passed 9/9 including a changed-generated-source cache-integrity negative, and a subsequent --ensure verified the output digest without rewriting the valid 1,063-root/1,724-function cache.

## Re-confirmed 2026-08-22

Post-commit Clang CTest passed 8/8; exact real-media gate executed BOOT and nested MENU with no recomp miss, traced 0x8002DE2C deterministically, and bootstrap passed 9/9 cache-integrity controls.

## Re-confirmed 2026-08-22

Against exact pinned psxport d2266f4b, the Clang real-disc gate passed on 136 runtime lines,
executed BOOT and nested MENU with no recompilation miss, and traced resident 0x8002DE2C;
full CTest passed 8/8.

## Re-confirmed 2026-08-22

Post-commit Clang CTest passed 8/8; exact real-media gate executed BOOT and nested MENU with no recomp miss, traced 0x8002DE2C deterministically, and bootstrap passed 9/9 cache-integrity controls.

## Re-confirmed 2026-08-24

Post-commit audit at recorded d2266f4b: 7/7 CTest passed; the recorded 136-line real-disc gate still provisions and executes BOOT and nested MENU before resident 0x8002DE2C.

## Re-confirmed 2026-08-27

Against exact recorded psxport `784e5212`, full Clang gates passed 107/107 in psxport and 11/11 in
Crash Bash. The serialized trace contains the measured order
`2/2 module loads -> empty prims -> MENU 0x800B5244 from ra=0x8001E7C0`, with no STUCK watchdog,
fatal output, or recompilation miss. Re-audit on 2026-08-27 found seven `VSync: timeout` lines that
the old positive fixture incorrectly accepted. The corrected 13/13 hermetic suite rejects those
lines.

## Re-confirmed 2026-08-27

The synchronous real-sector owner completed both requested load intervals from the verified CHD. The
strict 67-line product trace reaches `2/2 loads -> empty prims -> MENU 0x800B5244 from
ra=0x8001E7C0` with no timeout, fatal, watchdog terminal, recompilation miss, or guest-VSync
violation. This reconfirms module execution only; the subsequent direct run still presents black and
aborts at a later residual VSync owner.

## Re-confirmed 2026-08-30

Exact-identity idle/START/Cross gate passed: 88/88 idle/START updates, three ignored START edges, one Cross accept, DAT28136 callback 0x800B4694 installed and executed; its 10/10 falsifiers, 24/24 Clang CTest, 26 Python tests, and a 2400-frame no-miss/no-fatal/no-guest-VSync product extension passed.

## Re-confirmed 2026-08-30

Final exact-identity product differential passed 88/88 idle/START updates, three ignored START edges, one Cross accept, and DAT28136 callback 0x800B4694 installation/execution; final 10/10 judge, 24/24 Clang CTest, 26/26 Python tests, and retained 2400-frame no-failure extension passed.
