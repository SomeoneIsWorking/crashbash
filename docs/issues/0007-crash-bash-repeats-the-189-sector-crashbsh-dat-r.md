---
id: 7
title: Crash Bash repeats the 189-sector CRASHBSH.DAT read instead of completing
status: investigating
symptom: After continuous ReadN advances through LBA 35799..35987, the game restarts Setloc at LBA 35799 and never prints done loading
tags: boot,cdrom,libcd,dma,watchdog
created: 2026-08-21
updated: 2026-08-21
---

## Root cause

The controller in psxport `692b9b20` presented all continuous `ReadN` sectors synchronously from the
guest's BFRD acknowledgement path. Crash Bash therefore completed LBA 35799..35987, ran every nested
CD IRQ, and settled the read state before `FUN_8003470C` returned from starting the read. Its final
`FUN_80034384(0)` sees no operation in flight and returns zero. `FUN_80027790` treats that as a failed
start, returns `-1`, and the higher-level loader retries the same file range.

This was a shared drive-timing defect, not a game-local file-loader defect. Scheduling sector-ready
events in deterministic guest CPU time removes the retry. The resulting run exposes a second,
shared ordering defect: psxport coalesces the completion and clear causes into one CD interrupt drain,
so the guest cannot observe the pending state at the corresponding `0x800348A8` / `0x80027944`
return.

## Oracle/port comparison

The independent Beetle interpreter returns `1` from the read starter while requested/remaining are
both 189. The first `0x800348A8` result and `0x80027790` return are then 189. After LBA 35987,
remaining is zero and the async-completion flag is one, so `0x800348A8` and `0x80027944` return one
until the completion callback drains; the next sync returns zero with async clear, and the next read
is a different 16-sector range.

On psxport `692b9b20`, the starter instead returns zero after destination `0x800D7490` and expected
sector `0x8C94` prove all 189 payloads already landed. No `0x800348A8` or `0x80027944` event occurs
for this request. `0x80027790` returns `0xFFFFFFFF` four measured times with active, remaining, and
async all zero. `tools/verify_read_completion.py` accepts the paced oracle sequence and rejects this
instant-completion answer, a same-range restart, and wrong async state.

On current pinned psxport `3418a79b`, the initial boundary now agrees: the starter returns one at guest
instruction tick 1,332,412 with all 189 sectors pending, and `0x80027790` returns 189. Each
sector event is exactly 225,792 guest CPU cycles apart at mode `0xA0`; the file reaches
`done loading` without restarting and the next truthful stop is unloaded code at `0x80092BDC`.
The completion boundary is not yet oracle-equivalent. The port reaches
`v0=1, remaining=0, expected=0x8C94, async=1` transiently at cycle 44,081,261, but clears async at
44,081,771 before either measured function returns. The only observed sync return is
`v0=0, async=0` at 44,082,248. The strict comparator therefore still refuses the landed framework;
live
progress is not being substituted for state agreement.

Sparse write backtraces name the coalescing. Both writes occur inside one `0x8003F5F0` invocation:
the bit-4 path dispatches callback `0x80034040` and sets async to one, then the bit-2 path dispatches
`0x8003400C` and clears it before that IRQ handler returns. The true oracle returns sync/poll one
between those callbacks. The shared controller/event drain must therefore preserve two distinct
hardware response edges rather than returning both causes in one bitmask.

## What was tried / dead ends

The watchdog backtrace alone is misleading here because it fired while the process was decompressing
a CHD hunk for an actively advancing sector stream. Disabling the watchdog would hide both the
legitimate progress and the repeated-read bug. No game-local file-read override is warranted before
the guest result/state transition is measured.

The first shared pacing candidate delayed only following sectors on a host `steady_clock`. It removed
the repeated range and printed `done loading`, confirming the causal timing boundary, but the oracle
comparator rejected it: the first INT1 still ran inside the read starter, so `0x80027790` returned
188 instead of 189, and completion jumped directly to zero without the observable pending result
one. A debugger also changed the guest result by consuming host wall time before the guest call
returned. Correctness timing cannot depend on debugger latency or host load; the first sector and
completion must use deterministic emulated-time ordering.

A high-frequency GDB probe initially made the deterministic build hit the unchanged watchdog because
it trapped on every poll-loop return. A normal live run completed in under one second, and a later
state-word watch captured the exact guest-cycle sequence without changing the watchdog. That first
debugger stop was probe overhead, not evidence that the deterministic drive clock had stalled.

## Next investigation

Keep the deterministic drive schedule, then stop the shared CD response/IRQ path from coalescing the
bit-4 completion callback and bit-2 clear callback into one `0x8003F5F0` drain. The completion-pending
state must remain observable at `0x800348A8` and `0x80027944` between those hardware edges.
Resolution still requires the oracle's returned `1 -> 0` sequence, not merely `done loading` and no
retry. The deterministic framework is landed and pinned at `3418a79b`; after fixing the independent
response/IRQ ordering defect, emit/discover the loaded overlay entry `0x80092BDC`. Separately, route verified
CD/FMV progress into the shared watchdog owner rather than weakening its timeout.

## Rendering audit (2026-08-22)

The current shipping game code has no native graphics producer and cannot present a game picture yet.
With the verified USA disc and the binary built from the unchanged shipping `game/` sources against
the exact recorded psxport pin `3418a79b`, `tools/verify_boot.py` passed with 129 runtime lines: the
native GPU initialized, all 189 `CRASHBSH.DAT` sectors loaded, and dispatch then failed fast at
`0x80092BDC`. That address is loaded code outside every emitted resident/overlay range, not a renderer
regression. The same environment passed all 7 CTest gates.

Graphics work remains downstream of two real prerequisites: preserve the oracle-visible CD completion
`1 -> 0` ordering in the shared controller, then identify and emit the loaded executable containing
`0x80092BDC`. Adding a guest-packet fallback, a fabricated boot picture, or camera constants before that
execution spine exists would hide this boundary rather than advance the port.
