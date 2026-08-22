---
id: 9
title: Zero-latency GetTN response is drained before Crash Bash enters its poll state
status: investigating
symptom: After MENU executes and prints empty prims, the port never presents a frame and spins in resident 0x8002DE2C reading CD IRQ flag E0
tags: cdc,timing,interrupt,framework,first-frame
created: 2026-08-22
updated: 2026-08-22
---

## Root cause

The shared CDC executes command `0x13` synchronously on the command-register write and immediately queues INT3. During Crash Bash's `0x8002D4F4` send-state to poll-state transition, `rec_irq_poll` enters `0x8003E14C`, drains response bytes `02 01 01`, and acknowledges them. When the state machine reaches `0x8002DE2C`, the controller queue is already empty; every bank-1 IRQ-flag read returns `E0`.

## Evidence

`scratch/logs/crashbash-post-menu-cdregs-ad5cf802-final.log` records command -> handler response/ack ->
1,827,971 empty caller polls against exact pinned framework `ad5cf802`.
`tools/verify_command_response_timing.py` accepts that real trace and passes 5/5 controlled-answer
tests, rejecting a missing GetTN, wrong response, missing acknowledgement, and a caller-visible
response. `tools/verify_boot.py` proves both measured loaded modules executed before this boundary
with no recomp miss.

## Proper fix

Model command-response availability/order in guest time so a response cannot be fully handled during the command-write-to-poll-state transition. Preserve the existing deterministic sector schedule. A game-local CD override, a fake response, or watchdog relaxation would hide the framework defect and is prohibited.
