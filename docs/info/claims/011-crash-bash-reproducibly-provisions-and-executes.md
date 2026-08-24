---
id: C011
kind: claim
status: holds
created: 2026-08-22
tags: loaded-code,boot
depends: tools/loaded_module.py#verify_source, tools/provision.py#provision, tools/recomp_bootstrap.py#generated_measurement, tools/verify_boot.py#judge, titles/crashbash/boot_module.json, titles/crashbash/menu_module.json, game/recomp_seeds.json, psxport.pin
reconfirmed: 2026-08-24
verified_at: 2026-08-24 19:37:54
---

## Claim

Crash Bash reproducibly provisions and executes the measured BOOT and nested MENU CRASHBSH.DAT code modules before its next resident CD boundary

## Evidence

Real USA DAT verification passes 14/14 module facts; recompilation emits 1,063 roots into 1,724 resident/BOOT/MENU functions; the Clang real-disc boundary passes 8/8 facts and 7/7 controlled answers, executes ov_menu_gen_800B5218 with no recomp miss, and deterministically traces resident 0x8002DE2C.

## What would falsify it

If a verified USA DAT changes either payload identity/entry, generated ranges omit either module, or the real consumer no longer executes MENU before the next recomp/hardware boundary.

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
