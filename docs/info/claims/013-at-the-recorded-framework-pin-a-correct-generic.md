---
id: C013
kind: claim
status: holds
created: 2026-08-24
tags: cdc,timing,framework
depends: psxport.pin
---

## Claim

At the recorded framework pin, a correct generic CDC timing fix requires a pending-command phase machine rather than delayed visibility of a response prepared at command-register write time

## Evidence

At psxport d2266f4b, vendored Beetle PS_CDC_Write arms 10,500+jitter+1,815 ticks and PS_CDC_Update adds 1,815 per argument then 8,500 before executing the command. Read-only audit of the isolated delay candidate found its parameter formula short by 1,815 ticks, write-time Setloc/Setmode/ReadN/Pause/Stop/Reset effects, coalesced INT3/INT2 delivery, no pending-command busy phase, and command-before-drive tie ordering. docs/issues/0009 records the source lines, arithmetic, candidate defects, and required generic phase contract.

## What would falsify it

The vendored oracle command scheduler changes, or a differential controller trace proves write-time side effects, the shorter parameter timing, coalesced acknowledgement/completion, and command-before-drive tie ordering are hardware-equivalent
