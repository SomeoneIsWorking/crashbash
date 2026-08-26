---
id: 9
title: Zero-latency GetTN response is drained before Crash Bash enters its poll state
status: investigating
symptom: After MENU executes and prints empty prims, the port never presents a frame and spins in resident 0x8002DE2C reading CD IRQ flag E0
tags: cdc,timing,interrupt,framework,first-frame
created: 2026-08-22
updated: 2026-08-25
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

The boundary is unchanged on pinned psxport `d2266f4b`. A bounded headless register probe captured
GetTN, response bytes `02 01 01`, and the handler acknowledgement before 1,500 empty caller polls;
`tools/verify_command_response_timing.py` accepted the trace. Ghidra independently decompiled the
guest sequence: state-machine function `0x8002D4F4` calls command sender `0x8002E000`, returns, stores
the next state, and only then calls poller `0x8002DE2C`. The guest does not reorder the state store;
real hardware response latency is the missing operation.

## Rejected delay-only candidate (2026-08-24)

The isolated framework candidate under review correctly moves response visibility onto emulated CPU
time, but delaying a response prepared at command-register write time is not the controller contract.
The vendored Beetle oracle keeps the command and its argument FIFO in a pending phase machine. Its
`PS_CDC_Write()` arms `10,500 + jitter + 1,815` ticks; `PS_CDC_Update()` then spends another `1,815`
ticks for each argument before an `8,500`-tick execution phase. Therefore the first result for an
N-argument command is available after `10,500 + jitter + 1,815 + 1,815*N + 8,500` ticks. The
candidate's `10,500 + max(N, 1)*1,815 + 8,500` formula is short by 1,815 ticks for every command with
arguments (three-argument Setloc is 26,260 plus jitter, not 24,445).

The oracle also executes command side effects and samples status only when the execution phase
expires. The candidate instead changes Setloc, Setmode, ReadN, Pause, Stop, and Reset state at the
register write, clears the parameter FIFO immediately, and merely withholds precomputed bytes. It
also publishes each command's INT3 acknowledgement and INT2 completion together, does not expose
the command-busy phase in the status register, and services a due command before a simultaneously
due sector even though the oracle services the drive counter first. Those are controller-wide
ordering defects, not Crash Bash policy, so this candidate must not be used as the title milestone.

## Proper fix

Give the shared controller one pending-command phase machine driven by the existing authoritative
emulated CPU clock. A command write must latch the command and arguments and expose the busy/FIFO
state; deadline service must advance argument phases, execute side effects and sample response bytes
at the execution phase, raise the first INT3 only when the controller is ready, and schedule any
command-specific INT2 completion separately. When command and sector events share a deadline, retain
the oracle's drive-before-command order. Preserve the existing deterministic sector schedule and
test zero- and multi-argument delays, pre-deadline side-effect/status invariance, busy/FIFO bits,
INT3/INT2 separation, replacement by a newer command, and the command/sector tie.

`GameRuntime` at d2266f4b exposes no command-timing policy to override, and it should not: this is
shared hardware behavior. A title-only method, game-local CD override, delayed fake command, fake
response, or watchdog relaxation would hide the framework defect and is prohibited.

## Shared implementation status (2026-08-24)

The clean psxport `9c2e3f1c` worktree at `scratch/wt/cdc-command-phases` now implements the proper
shared phase machine, not the rejected delay-only candidate. `cdc_command_phase.{h,cpp}` owns the
12,315-tick write/receive phase, 1,815-tick argument transfers, and 8,500-tick execution phase.
`cdc_native.cpp` performs command effects and samples status only at execution, holds multi-phase
INT2 completion until the preceding IRQ is acknowledged, exposes BUSYSTS, and services drive events
before command events on an exact tie. `Setloc` now owns a command target separate from the physical
head, so deterministic seek timing follows the vendored oracle formula instead of a fixed shortcut.

`test_cdc_command_phases` passes 8/8 cases and 48 checks, the focused CDC regression pair passes, and
the full Clang framework CTest passes 97/97. A separate `BUILD_TESTING=OFF` Crash Bash integration
tree configured with Clang 22.1.8 against that worktree, contained no CTest files, and linked only the
real `crashbash_port` product target. Because this task is explicitly forbidden from executing the
game binary, the real consumer has not yet falsified or confirmed the predicted GetTN handoff; this
issue remains investigating until a bounded runtime trace shows resident `0x8002DE2C` observing INT3
and advancing to the next divergence.

## Landed framework and consumer gate (2026-08-25)

The generic phase machine is pushed in psxport `8611d756` after the combined Clang framework gate
passed 100/100, and this consumer now records that exact pin. The earlier dirty candidate trace
`scratch/logs/crashbash-cdc-phases-once.log` already provides the expected opposite answer: one GetTN
returns `02 01 01`, the former `0x8002DE2C` poll occurs zero times, and execution advances through
6 Setloc, 1 Setmode, 6 ReadN, and 5 Pause commands into continuous ranges 35799..35987 and
17558..17655. That trace proves the candidate behavior but is not clean landed-product evidence.

`tools/verify_cdc_phase_progress.py` now owns the clean consumer gate. It requires those exact command
denominators, response bytes, zero old polls, both continuous ranges, and no fatal/miss/timeout/
unhandled-command/controller-zero failure. It stops only its exact child PID after positive target
LBA 17655; reaching its wall-clock timeout refuses. Its hermetic and subprocess controls now pass
8/8, including the response-edge negative below. This issue remains investigating until that
verifier passes against the clean product built from the recorded framework pin.

## Response-edge milestone (2026-08-26)

The recorded consumer pin is now `17981527`, a descendant of `8611d756` with no changes to
`cdc_native.cpp`, `cdc_command_phase.cpp`, `cdc_command_phase.h`, or `cdc_state.h`. At that pin,
phase-2 completion is held while the response queue is nonempty; acknowledging a response increments
`irq_sequence` when the next queued response becomes current. The existing 8,606-line candidate trace
shows that mechanism in the real title path: all 5 Pause commands expose INT3 acknowledgement and
INT2 completion under distinct `0x8003F5F0` handler entries.

`tools/verify_cdc_phase_progress.py` now requires those 5/5 separated response pairs in addition to
the GetTN and sector-progress contract. Its 8/8 controls include a coalesced fixture with the second
handler entry removed, which is rejected. This rules out the old single-handler response coalescing
mechanism in the candidate trace; it does not prove that the clean pinned product exposes Crash
Bash's guest-visible async result `1 -> 0`. C015 records that deliberately narrower claim.
