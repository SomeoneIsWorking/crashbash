---
id: I005
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

Crash Bash tools/verify_boot.py port-side CD-interrupt service verifier

## Validated by

It launched the real Clang-built port against the verified executable, accepted six required boundary facts plus the ordered 0x80031AE8 -> 0x80031B58 -> 0x8003F5F0 -> 0x8003E14C service, required `load file start`, and excluded recomp misses, segmentation faults, the former CD timeout, and `Cant find CRASHBSH.DAT`. It then rejected controlled traces with guest-main reachability removed, an injected recomp miss, the master-dispatcher address broken, and file-load progression removed (5/5).

## Known failure modes

This verifier judges a bounded port trace, not guest memory equality against a true oracle. The paired true-oracle comparison is owned by instrument I006; neither instrument proves full-RAM lockstep or correctness after the CD callback drains its response.
