---
id: I005
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

Crash Bash `tools/verify_boot.py` deterministic loaded-module/MENU boundary verifier

## Validated by

Its default CTest mode is a hermetic judge/runner selftest and never launches the game. Explicit
`--run` launches the real Clang-built port against the verified executable and stops only that exact
child after the positive boundary. The judge requires exactly two ordered `load file start` / `done
loading` pairs, then `empty prims`, then exactly one game-owned marker proving measured MENU entry
`0x800B5244` from `ra=0x8001E7C0`. That marker wraps and immediately super-calls the retained
generated body; it is not an async stack sample or unsupported generic trace knob. The judge rejects
STUCK/INTERRUPT watchdog terminals, fatal output, recompilation misses, segmentation faults, the
former CD timeout, every `VSync: timeout`, and a missing DAT.

The controlled suite passes 13/13, including missing/wrong/reordered module and MENU facts, a real
STUCK line, a guest-VSync-timeout negative, and an exact-child cleanup whose SIGTERM handler emits
`[watchdog] INTERRUPT` only after the positive boundary. Cleanup output is deliberately excluded
from pre-boundary product evidence. The old serialized exact-pin trace still contains the ordered
2/2 loads, `empty prims`, and exact MENU entry/caller marker, but the corrected judge refuses it for
seven `VSync: timeout` lines. The current 67-line post-native-ownership product trace provides the
other answer and passes with no forbidden terminal or guest-VSync output.

## Known failure modes

This verifier judges a bounded port trace, not guest memory equality against a true oracle. It stops
at MENU entry and therefore proves neither completion of that generated body nor a presented game
frame. The paired true-oracle comparison is owned by instrument I006; command-response ordering is
owned by I008.
