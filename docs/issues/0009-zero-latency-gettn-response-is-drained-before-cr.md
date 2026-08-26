---
id: 9
title: Exact-pin Crash Bash reaches MENU but presents no game frame
status: investigating
symptom: The clean current product completes both loaded modules and reaches the resident/BOOT/MENU chain, then presents zero game frames before the watchdog
tags: cdc,timing,interrupt,framework,first-frame
state_items: S002,S007
created: 2026-08-22
updated: 2026-08-27
---

## Historical root cause on the pre-phase-machine framework

Before the shared command-phase machine landed, the CDC executed command `0x13` synchronously on the
command-register write and immediately queued INT3. During Crash Bash's `0x8002D4F4` send-state to
poll-state transition, `rec_irq_poll` entered `0x8003E14C`, drained response bytes `02 01 01`, and
acknowledged them. When the state machine reached `0x8002DE2C`, the controller queue was already
empty; every bank-1 IRQ-flag read returned `E0`. The clean current-pin run below does not carry the
diagnostics required to establish that this historical mechanism recurred.

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

## Proper fix for the historical cause

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

The then-recorded consumer pin was `17981527`, a descendant of `8611d756` with no changes to
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

The current recorded pin is `99a42aa3`. Its static Clang gate passes 10/10, but it includes the later
shared XA/CDC commit `f9b5db8f`; therefore the `17981527` candidate trace is not runtime evidence for
the current product. The operator-owned serialized CDC/completion run remains required.

The positive verifier is a process-driving gate, not a positional-log parser. Its clean invocation is
`uv run --frozen python tools/verify_cdc_phase_progress.py --port
scratch/bin/crashbash_port --executable scratch/bin/crashbash/SCUS_945.70 --timeout 30`; it launches
and stops only that exact child and refuses unless it observes positive LBA 17655. A previously saved
trace is judged only with the explicit `--trace PATH` option. The render-anchor log below did not
enable the CDC diagnostics and cannot satisfy this contract.

## Clean exact-pin first-frame falsifier (2026-08-26)

The operator ran the clean product built against exact recorded psxport `99a42aa3` with
`PSXPORT_DEBUG=rtpcaller` and a 30-second watchdog. The process exited 134 after the watchdog reported
`[watchdog] STUCK`; `tools/verify_render_anchor_reach.py` correctly refused that forbidden terminal
and the absence of any completed 50-frame histogram. The exact output is retained locally at
`scratch/logs/crashbash-render-anchor-reach.log`.

This is a real current-pin result: it opens the retail disc, completes both `load file start` / `done
loading` pairs, enters resident code, executes BOOT and nested MENU, prints `empty prims`, and presents
zero game frames. The watchdog sampled this symbolized path; only the leading libc/signal and
`std::string_view` frames are omitted, while the generated/dispatch ancestry is retained exactly:

`lucent::detail::channel_enabled -> lucent::debug -> Core::io_write ->
gen_func_8002DE2C -> func_8002DE2C -> gen_func_8002D4F4 -> func_8002D4F4 -> rec_dispatch ->
ov_boot_gen_8008E5BC -> ov_boot_func_8008E5BC -> rec_dispatch ->
ov_menu_gen_800B5218 -> ov_menu_func_800B5218 -> rec_dispatch ->
gen_func_8001E610 -> func_8001E610 -> rec_dispatch ->
ov_boot_gen_80092BA0 -> ov_boot_func_80092BA0 -> rec_dispatch ->
gen_func_80010394 -> func_80010394 -> rec_dispatch ->
gen_func_800270F0 -> func_800270F0 -> gen_func_80010158 -> func_80010158 ->
gen_func_8002718C -> func_8002718C -> rec_dispatch -> native_boot_run -> main`.

The signal-time backtrace proves reachability of that live nested chain, not that logging is the
guest divergence or that `0x8002DE2C` is continuously spinning. The early CVar audit also reported
`PSXPORT_NATIVE_FRAMES` and `PSXPORT_VK_HEADLESS` as UNKNOWN before the stall; because the abort
prevented an exit audit, this run cannot claim either knob was honored. It contains no `rtpcaller`
histogram, no CDC-phase trace, and no guest-visible completion capture, so issues 0007/0009 remain
investigating at the no-present boundary rather than reverting to the older GetTN root-cause claim.

## Dirty unpinned sampling (non-authoritative, 2026-08-26)

During a Clang candidate build configured against the shared psxport tree while its worktree was
dirty at HEAD `dbdb2baf`, `ctest --test-dir scratch/build/crashbash-progress --output-on-failure -E
crashbash_psxport_pin` unintentionally included the product-launching
`crashbash_boot_boundary_selftest`. The bounded child exited by itself after 3.24 seconds and the
judge refused its 135-line trace: it observed two loaded-module starts but only one completion and
did not reach MENU `empty prims`, `ov_menu_gen_800B5218`, or resident fntrace `0x8002DE2C`.

This is recorded only so the observation is not rediscovered or misquoted. It is not exact-pin or
clean-framework evidence, it does not supersede the recorded-pin result, and it does not establish a
regression. The operator-owned serialized run must rebuild against one clean recorded commit and run
the positive CDC/completion gate before this boundary can change state.

## Watchdog phase root cause and corrected boot boundary (2026-08-27)

The normal boot gate's post-MENU watchdog was not evidence of a CDC stall. `native_boot_run` calls
`gpu_clear_display` for an intentional black transition before guest crt0; that helper used the same
untyped completion path as a real main game frame and prematurely ended the 45-second boot grace.
The smallest shared fix classifies presentation completion as `MainFrame` or `Transition`:
`gpu_clear_display` records progress without claiming main-present readiness, while real presents
retain the one-way steady-timeout transition. It adds no CDC heartbeat and changes no timeout.

Landed psxport `784e5212` passes 107/107 Clang CTests, including a production-seam
`test_gpu_clear_watchdog` that was killed by the steady timeout before the fix and now survives under
boot grace. Crash Bash passes 11/11 CTests against that exact recorded pin. Its explicit serialized product
gate passes on 72 judged lines: two ordered module loads complete, `empty prims` prints, then a
game-owned one-shot observer reaches measured MENU entry `0x800B5244` from `ra=0x8001E7C0`; no
`[watchdog] STUCK`, fatal output, or recompilation miss occurs. The observer immediately super-calls
the retained generated MENU body and does not bypass game behavior.

This fixes the false watchdog phase transition and makes the boot boundary deterministic. It does
not resolve this issue's CDC/completion contract or prove a game frame: the run intentionally stops
at the MENU entry and carries no
CDC-phase diagnostics. The next evidence remains the process-driving CDC gate and capture of the
retail guest-visible completion result `1 -> 0`.
