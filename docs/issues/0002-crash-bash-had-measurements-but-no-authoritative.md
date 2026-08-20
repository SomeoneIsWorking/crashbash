---
id: 2
title: Crash Bash had measurements but no authoritative provisioner
status: resolved
symptom: Manual extraction could produce an executable without binding disc selection, SYSTEM.CNF, the tracked manifest, and published output into one checked operation
tags: boot,provisioning
created: 2026-08-21
updated: 2026-08-21
---

## Root cause

The scaffold recorded a measured executable and manual discdump commands, but no shipping tool owned source precedence or compared extracted media to that measurement before publication.

## Resolution

`tools/provision.py` now resolves one authoritative disc, extracts into scoped scratch, checks SYSTEM.CNF plus all 11 executable identity/header facts, and atomically publishes only after success. `tests/test_provision.py` covers the matching, mismatch, refusal, precedence, ambiguity, prior-output-preservation, and independent failure of every tracked fact.
