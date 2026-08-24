---
id: C013
kind: claim
status: holds
created: 2026-08-24
tags: cdc,timing,framework
depends: docs/issues/0009-zero-latency-gettn-response-is-drained-before-cr.md
reconfirmed: 2026-08-25
verified_at: 2026-08-25 01:10:38
---

## Claim

At the recorded framework pin, a correct generic CDC timing fix requires a pending-command phase machine rather than delayed visibility of a response prepared at command-register write time

## Evidence

At psxport d2266f4b, vendored Beetle PS_CDC_Write arms 10,500+jitter+1,815 ticks and PS_CDC_Update adds 1,815 per argument then 8,500 before executing the command. Read-only audit of the isolated delay candidate found its parameter formula short by 1,815 ticks, write-time Setloc/Setmode/ReadN/Pause/Stop/Reset effects, coalesced INT3/INT2 delivery, no pending-command busy phase, and command-before-drive tie ordering. docs/issues/0009 records the source lines, arithmetic, candidate defects, and required generic phase contract.

## What would falsify it

The vendored oracle command scheduler changes, or a differential controller trace proves write-time side effects, the shorter parameter timing, coalesced acknowledgement/completion, and command-before-drive tie ordering are hardware-equivalent

## Re-confirmed 2026-08-24

A clean psxport 9c2e3f1c worktree now implements the audited phase contract in cdc_command_phase and cdc_native: 8/8 shipping phase tests (48 checks), focused continuous-read regression, and full Clang framework CTest 97/97 pass. No Crash Bash binary was executed, so this confirms the generic implementation shape but not the consumer handoff.

## Re-confirmed 2026-08-25

The same generic implementation is pushed as psxport `8611d756` after the combined Clang framework
gate passed 100/100, and `psxport.pin` records that exact commit. The locked positive-progress
verifier passes 7/7 controlled answers and accepts the existing candidate trace, while a clean pinned
consumer run remains the stated falsifier before landed runtime behavior can be claimed.
