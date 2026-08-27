---
id: I012
kind: instrument
status: trusted
created: 2026-08-27
---

## Instrument

Crash Bash `tools/verify_native_ownership.py` retail-byte and shipping-wiring verifier

## Validated by

The verifier reads the verified `SCUS_945.70`, decodes the 21 retail JAL instructions in the
migrated display/allocator, synchronous-GPU, memory-card, and CD owners that target libetc VSync
`0x800320EC`, and
compares the three words at process-state table `0x8004E0B8` with the typed enter/update/present
declarations. It also requires the exact four-byte platform trap window, the Crash Bash FrameDriver
factory, finite boot prefix, process-runner activation, display delivery, the GPU overrides and
their retained supers, absence of a GPU timing query/poll loop, and the one presented/unpresented
commit seam in shipping source. The real-input path additionally checks the complete opcode
denominator: 47 resident JALs plus four in the verified BOOT payload.

Its controlled suite passes 7/7: one consistent image/wiring positive plus mutated process table,
mutated VSync JAL, missing fatal-trap, GPU-timing-query, memory-card-VSync-dispatch, and
CD-license-direct-VSync negatives. The real verified executable
also passes. This shows the other answer and binds the static ownership declaration to retail bytes;
it does not run the game or prove that the new frame path reaches MENU or presents a correct picture.
