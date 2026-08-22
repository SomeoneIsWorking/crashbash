---
id: 10
title: Watchdog sample made the Crash Bash boot verifier report a missing poll function
status: resolved
symptom: crashbash_boot_boundary_selftest intermittently refuses missing _Z17gen_func_8002DE2CP4Core even though execution reaches the resident poll state
tags: harness,watchdog,recompiler,boot
created: 2026-08-22
updated: 2026-08-22
---

## Root cause

The verifier treated the asynchronous watchdog backtrace as deterministic guest reachability evidence. Depending on signal timing, the same valid run was sampled either inside `gen_func_8002DE2C` or its caller `gen_func_8002D4F4` while servicing guest instruction ticks. A/B proved `generated/shard_1.c` contains the body and the linked binary exports the symbol; generation was not the observed failure.

## Resolution

`tools/verify_boot.py` now function-traces guest address `0x8002DE2C` and requires its deterministic REACHED marker. The watchdog remains required only as the terminal no-frame boundary. A controlled negative removes the poll trace, and the live verifier passes 7/7. Generated-cache inspection separately found that existence-only completeness could accept a changed shard, so the bootstrap now records and verifies a digest across every shipping generated source/interface and rejects a changed-source negative. Full Clang CTest passes 8/8.
