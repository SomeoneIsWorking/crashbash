---
id: C010
kind: claim
status: holds
created: 2026-08-22
tags: architecture,runtime
depends: game/core/crashbash_runtime.cpp, game/core/main.cpp, game/core/game_hooks.cpp, CMakeLists.txt
reconfirmed: 2026-08-22
verified_at: 2026-08-22 18:30:27
---

## Claim

Crash Bash framework-facing behavior is owned by a process-lifetime derived CrashBashRuntime while GameConfig and GameHooks are bounded compatibility facts only

## Evidence

On psxport ad5cf802, main installs CrashBashRuntime before constructing Game; bootInit and
registerOverrides are virtual overrides on that class, and the legacy hook table is empty. The real-disc
consumer provisions and executes BOOT plus nested MENU through the retained generated bodies, then
reaches the resident GetTN polling boundary at 0x8002DE2C without a recomp miss. Full Clang CTest
passes 8/8, including format, the 1,200-line structure cap, compile-database clang-tidy, provenance,
and controlled-negative harnesses.

## What would falsify it

A shipping behavior is added to GameConfig/GameHooks, the runtime is not installed before Core construction, framework code bypasses the runtime virtuals, or the real-disc boot boundary changes

## Re-confirmed 2026-08-22

Against pinned psxport ad5cf802, CrashBashRuntime remains installed before Game construction and owns bootInit/registerOverrides; the real-disc consumer executes BOOT and nested MENU through retained generated bodies, reaches resident 0x8002DE2C without a recomp miss, and full Clang CTest passes 8/8.

## Re-confirmed 2026-08-22

Post-commit Clang CTest passed 8/8; CrashBashRuntime remains installed before Core and owns behavior while legacy GameHooks is empty.
