---
id: I005
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

Crash Bash tools/verify_boot.py port-side CD-interrupt service verifier

## Validated by

It launches the real Clang-built port against the verified executable, requires the ordered
0x80031AE8 -> 0x80031B58 -> 0x8003F5F0 -> 0x8003E14C service, `load file start`, `done loading`, and
the measured next boundary at unloaded entry 0x80092BDC. It excludes an earlier or different recomp
miss, segmentation faults, the former CD timeout, and `Cant find CRASHBSH.DAT`. Controlled traces
with guest-main reachability removed, an injected unexpected miss, the master-dispatcher address
broken, or file-load progression removed all fail (5/5).

## Known failure modes

This verifier judges a bounded port trace, not guest memory equality against a true oracle. The
0x80092BDC miss is required because it is the current honest overlay-discovery boundary, not accepted
as successful boot. The paired true-oracle comparison is owned by instrument I006; neither instrument
proves full-RAM lockstep or correctness after file completion.
