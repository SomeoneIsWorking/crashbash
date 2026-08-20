---
id: C001
kind: claim
status: holds
created: 2026-08-20
tags: boot,executable
depends: titles/crashbash/executable.json
reconfirmed: 2026-08-21
verified_at: 2026-08-21 02:35:28
---

## Claim

Crash Bash targets the North American SCUS-94570 disc whose boot executable is SCUS_945.70 with entry 0x8002E7B0

## Evidence

Real CHD: SYSTEM.CNF names cdrom:\\SCUS_945.70;1; executable has North America and BASCUS-94570 markers; 432128-byte image SHA-256 fd5727a18feb2a2d5a6359a55966f0266284d1e50f64ee9b8a127a97091bd516; PS-X EXE header load 0x80010000 text 0x69000; crt0_extract decoded 36 instructions and resolved 8/8 fields

## What would falsify it

a newly provisioned USA disc has a different SYSTEM.CNF boot target, executable hash, region markers, or PS-X EXE header

## Re-confirmed 2026-08-21

Reprovisioned the real USA CHD through tools/provision.py: SYSTEM.CNF boot 1/1 and executable identity/header 11/11; published output SHA-256 fd5727a18feb2a2d5a6359a55966f0266284d1e50f64ee9b8a127a97091bd516.
