---
id: C017
kind: claim
status: holds
created: 2026-08-26
tags: rendering,native,widescreen,interpolation
depends: game/core/crashbash_runtime.h#CrashBashRuntime::renderCapabilities
---

## Claim

Crash Bash declares native rendering, temporal interpolation, and widescreen-capable native presentation through the title-owned runtime seam

## Evidence

CrashBashRuntime::renderCapabilities returns RenderCapabilities::interpolatedNative(); exact recorded psxport 784e5212 Clang links crashbash_port, the shared C++ policy including clang-format/clang-tidy passes, the pin gate passes, and all 11 CTests pass. This proves capability policy wiring only, not a native producer or runtime picture.

## What would falsify it

CrashBashRuntime stops returning interpolatedNative, the framework changes that profile's native/temporal semantics, or the exact-pin Clang/static gate fails
