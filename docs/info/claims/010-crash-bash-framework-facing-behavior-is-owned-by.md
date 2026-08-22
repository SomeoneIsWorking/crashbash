---
id: C010
kind: claim
status: holds
created: 2026-08-22
tags: architecture,runtime
depends: game/core/crashbash_runtime.cpp, game/core/main.cpp, game/core/game_hooks.cpp, CMakeLists.txt
---

## Claim

Crash Bash framework-facing behavior is owned by a process-lifetime derived CrashBashRuntime while GameConfig and GameHooks are bounded compatibility facts only

## Evidence

On psxport 7f5d3f13, main installs CrashBashRuntime before constructing Game; bootInit and registerOverrides are virtual overrides on that class, the legacy hook table is empty, and the 129-line real-disc boot verifier preserved all 7 required DAT/IRQ facts plus exact next miss 0x80092BDC. Full Clang CTest passed 7/7 including format, 1200-line structure, compile-database clang-tidy, pin, and controlled-negative harnesses.

## What would falsify it

A shipping behavior is added to GameConfig/GameHooks, the runtime is not installed before Core construction, framework code bypasses the runtime virtuals, or the real-disc boot boundary changes
