---
id: I005
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

Crash Bash tools/verify_boot.py port-side CD-interrupt service verifier

## Validated by

It launches the real Clang-built port against the verified executable plus measured BOOT and MENU
modules. It requires the ordered 0x80031AE8 -> 0x80031B58 -> 0x8003F5F0 -> 0x8003E14C service, two
`load file start` / `done loading` pairs, `empty prims`, generated MENU execution, and the measured
resident boundary at 0x8002DE2C. It excludes any recomp miss, segmentation fault, the former CD
timeout, and `Cant find CRASHBSH.DAT`. Its selftest accepts the real boundary and rejects five
controlled mutations plus removal of the resident-poll trace (7/7). The poll-state reachability fact
comes from deterministic guest function tracing; the asynchronous watchdog backtrace is used only as
the terminal no-frame boundary and is not treated as a stable sampling location.

## Known failure modes

This verifier judges a bounded port trace, not guest memory equality against a true oracle. The paired
true-oracle comparison is owned by instrument I006; neither instrument proves full-RAM lockstep or
correctness after the resident 0x8002DE2C boundary. Command-response ordering there is owned by I008.
