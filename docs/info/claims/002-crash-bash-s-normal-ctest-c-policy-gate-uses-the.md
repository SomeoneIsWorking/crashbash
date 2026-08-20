---
id: C002
kind: claim
status: holds
created: 2026-08-21
tags:
depends: CMakeLists.txt
reconfirmed: 2026-08-21
verified_at: 2026-08-21 02:21:55
---

## Claim

Crash Bash's normal CTest C++ policy gate uses the shared psxport checker; its empty scaffold produces explicit zero denominators, and CMake refuses a GNU C++ compiler before configuring

## Evidence

Clang 22 configure plus the scoped CTest passed; a fresh CC=gcc CXX=g++ configure failed at the project compiler guard; the checker reported 0 format, 0 size, and 0 touched clang-tidy units

## What would falsify it

A first-party C++ source is added without becoming format/tidy/size checked, a non-Clang compiler configures successfully, or the shared checker/pin changes without re-verification

## Re-confirmed 2026-08-21

Reverified Crash Bash from a fresh Clang 22 build against clean pinned psxport eb2465b2: scaffold linked, psxport_smoke passed 8/8, scoped CTest policy passed with explicit empty-scaffold denominators, and psxport_sync.py --check confirmed the built SHA equals the recorded pin.

## Re-confirmed 2026-08-21

Reverified Crash Bash after final framework landing be381503: CMake reconfigured with Clang 22, scaffold rebuilt and linked, psxport_smoke passed 8/8, normal CTest C++ policy passed, and psxport_sync.py --check confirmed build/ used the exact recorded be381503 pin.
