---
id: C002
kind: claim
status: holds
created: 2026-08-21
tags:
depends: CMakeLists.txt
reconfirmed: 2026-08-21
verified_at: 2026-08-21 03:42:29
---

## Claim

Crash Bash's normal CTest C++ policy gate uses the shared psxport checker for every first-party translation unit, and CMake refuses a non-Clang C++ compiler before configuring

## Evidence

Clang 22 built the four first-party seam translation units and the generated product; the normal CTest checker reported 4/4 format, 4/4 size, and 4/4 compile-database clang-tidy units. The established forced-negative GNU configure still exercises the CMake compiler guard.

## What would falsify it

A first-party C++ source is added without becoming format/tidy/size checked, a non-Clang compiler configures successfully, or the shared checker/pin changes without re-verification

## Re-confirmed 2026-08-21

After the first Crash Bash seam landed in the worktree, Clang 22 built all four first-party TUs against psxport 2b5ef7b5 and normal CTest reported 4/4 format, 4/4 size, and 4/4 compile-backed clang-tidy checks.

## Re-confirmed 2026-08-21

Reverified Crash Bash from a fresh Clang 22 build against clean pinned psxport eb2465b2: scaffold linked, psxport_smoke passed 8/8, scoped CTest policy passed with explicit empty-scaffold denominators, and psxport_sync.py --check confirmed the built SHA equals the recorded pin.

## Re-confirmed 2026-08-21

Reverified Crash Bash after final framework landing be381503: CMake reconfigured with Clang 22, scaffold rebuilt and linked, psxport_smoke passed 8/8, normal CTest C++ policy passed, and psxport_sync.py --check confirmed build/ used the exact recorded be381503 pin.

## Re-confirmed 2026-08-21

After provisioning CMake integration, Clang 22 rebuilt crashbash_scaffold and discdump; scoped normal CTest passed crashbash_cpp_policy plus crashbash_provision_selftest 2/2, and psxport_sync.py --check matched built framework be381503 to the recorded pin.

## Re-confirmed 2026-08-21

Post-landing scoped Crash Bash CTest passed 5/5; cpp_policy format/size checked four first-party files and clang-tidy used all four Clang compile commands.
