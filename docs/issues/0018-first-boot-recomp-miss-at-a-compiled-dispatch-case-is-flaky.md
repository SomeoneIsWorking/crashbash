---
id: 18
title: Two appended builds made an ordinary missing target look like a flaky compiled-case miss
status: resolved
symptom: Reused flow-probe logs appeared to show 0x80012840 fail in a binary whose generated MAIN dispatch already contained that case, leading to a false router-race diagnosis.
state_items: S002,S003,S007
tags: recomp-miss,provenance,dispatch,attract,diagnostic-falsified
created: 2026-08-29
updated: 2026-08-30
---

## Root cause

The evidence combined two different binaries and then inspected a third source state:

- Lucent opens `PSXPORT_LOG_FILE` in append mode. Both `flow-probe8.log` and `flow-probe9.log`
  start with failing build `d2c4465-dirty+psxport-02430b1b-dirty`, miss `0x80012840`, then contain
  another complete startup for clean build `9edf471-dirty+psxport-625f8e69`.
- Commit `d2c4465` does **not** contain a `0x80012840` seed. Commit `9edf471` is precisely the change
  that adds it. The failure was therefore the ordinary discovery miss that motivated the seed; the
  later binary contained the case and ran cleanly.
- `generated/` is ignored. Inspecting its regenerated `shard_disp.c` after the later emission cannot
  establish which switch the older binary compiled.

The old issue text called the second process an automatic reboot, called the address already compiled,
and inferred an IRQ race from `c->pc=0x80027944`. All three claims were false. Generated wrappers use
`Core::pc` as the last function entered and intentionally do not restore it; routing never consults
that field. The current wrapper owner is the attribution stack, not `Core::pc`.

## Resolution

psxport now fingerprints the exact emitted translation units and routing metadata, compiles that
identity into `g_rec_substrate_id`, and exposes it through `RecompRegistry`. Every installed substrate
announces the full identity; absence is reported as `UNKNOWN`. Crash Bash's bootstrap binds the stamp,
compiled table, generated header, output cache, and shipping registry adapter, with a forced mismatch
negative. Its boot verifier also requires the exact current identity once, before the first module
load, and rejects missing and stale identities through the shipping judge.

The unconditional 128-entry dispatch-decision ring added solely for the false race was removed. The
miss diagnostic now labels `Core::pc` as `last-fn-entered` and prints the current wrapper owner. Generic
MAIN return-boundary discovery independently derives `0x80012840`, so no replacement title seed is
needed.
