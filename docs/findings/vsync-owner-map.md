# Crash Bash guest-VSync owner map

The verified USA resident image contains 47 MIPS `JAL 0x800320EC` instructions in 20 containing
roots. The verified BOOT payload adds four direct calls in two roots. The linked guest-image total
is therefore 51 call sites across 22 containing roots. This count comes from retail opcodes, not a
generated-C regex: generated roots can contain a second, unseeded MIPS function after an emitted C
`return`, while BOOT calls use overlay-local dispatch and were absent from the earlier resident-only
count.

| Runtime owner | Containing roots | Retail VSync call sites | Native state |
|---|---|---:|---|
| Display and frame arena | `0x800272AC`, `0x80010488` | 5 | owned by `display_frame.cpp` |
| GPU synchronous timeout/transfer | `0x8003126C`, `0x800312A0`, first callable function in `0x8003165C` | 3 | owned by `gpu_timeout.cpp` |
| Memory-card BIOS/vector startup | `0x800486DC` | 1 | owned by `memory_card_startup.cpp`; the first post-change product trap proved its boot-time reachability through `0x80027F00 -> 0x8002C894 -> 0x8003ABAC` |
| Synchronous CD readiness/startup/command | `0x800349AC`, `0x80034B8C`, `0x8003E6B0`, `0x8003EBF8` | 6 | the title-local readiness owner requires a real parsed CHD TOC and the controller handshake reports host readiness; measured command/sync entries route to psxport's synchronous CD owner and never enter their VSync timeout queries |
| Synchronous disc/license startup | `0x8002D4F4` | 6 | `cd_license_startup.cpp` binds the runtime medium to the measured SCUS-94570 track, file extents, and SYSTEM.CNF, then records the retail authentic-disc state 0; its generated 20-state controller body remains the A/B super |
| Other GPU library fragments | two hidden functions following `0x800313E4`; hidden function following the callable `0x8003165C` body | 3 | residual; generated root coverage must be split before they can execute faithfully |
| Other CD controller and libcd paths | `0x80034040`, `0x80034384`, `0x8003470C`, `0x800348A8`, `0x8003E930`, `0x8003F47C` | 19 | residual; port only a top-down path proven reachable after the measured command/sync chokepoints |
| Lifetime, diagnostics, teardown, UI | `0x800101E0`, `0x80027408`, `0x8002AB44` | 4 | residual; migrate only when its top-down caller is proven reachable |
| BOOT object callbacks | `0x8008ADA4`, `0x8008BB48` | 4 | owned by `boot_object_callbacks.cpp`; a live 600-frame run confirmed both are reached once the BOOT scene begins animating an object, which the static inventory could not prove |

## Polar Push DAT22510 contact pass — residual title ownership

The Polar Push module adds a separate, loaded-overlay residual. Its first reached call is
`DAT22510 0x800CDA54 -> 0x800C0888 -> VSync(1)`, with the trap reporting return address
`0x800C08E0`. The exact DAT22510 bytes contain the JAL at `0x800C08D8`; the helper has two more
`VSync(1)` sites at `0x800C0E24` and `0x800C0E54`.

`0x800CDA54` is one of the per-player arena-state handoffs. For player `i`, it obtains the
`0x6C`-byte player record, advances the state helpers, then calls `0x800C0888(record[i].position +
0x10, i)` before its next state handler. The helper walks `DAT_800D6010`, filters active entries,
tests a 330-unit X/Z contact radius around that player, adjusts the player-associated motion state,
and can spawn the `0x1604` contact effects before clearing the consumed entry's active bit. Its
other callers are the sibling player-state handoffs, so this is an arena contact/effect update, not
the generic presentation cadence.

The owner contract is now statically resolved: `DAT_800D6010` is an ascending 256-pointer contact
table; each contact is `{flags, x, y, z}` at offsets `0/+4/+8/+12`, and bit `0x8000` is active. Player
slots start at `0x8009D52C + 0x6C*i`; the entity pointer is `+4`, auxiliary motion pointer `+0x18`,
and configuration index `u16 +0x2E`. A contact passes only when `abs(dx) < 331`, `abs(dz) < 331`, and
`dx*dx + dz*dz < 108900` (so 108899 is inside and 108900 is outside). A geometric pass either retains
the active bit after motion adjustment or consumes it after the threshold branch, which spawns three
`0x1604` effects and performs the associated event/choreography writes. The first passing table entry
returns immediately. A native owner needs typed player/contact/motion views, the existing math/effect
owners, a generated A/B mirror over every touched guest range, and explicit counters for scanned,
active, disk-pass, retained, consumed, and emitted effects.

All three `VSync(1)` return values are dead in this helper: the entry value is stored at `sp+0x18`
and never loaded, and the two exit values are likewise stored at `sp+0x1C` and never loaded. That
proves neither a wait nor a guest time value is needed for this path; it does **not** authorize a
VSync success override. The framework trap remains correct. A future native
`PolarPushArenaContactUpdate` must own the complete helper behind the existing
`CrashBashFrameDriver` (one frame/presentation owner), retain its generated body as the A/B super,
and omit only the three dead samples while preserving their instruction ticks. Static evidence alone
does not establish the dynamic record invariants for that port, so no partial override was installed.

`0x8008BB48` samples `VSync(1)` at entry, calls `0x8008ADA4`, and samples again before its normal
return; `0x8008ADA4` also samples at entry and return. A live trap at `ra=0x8008BB88` via
`0x8007976C -> 0x8008BB48` later confirmed the path, and both functions are now natively owned.

All four are DEAD samples, which the retail disassembly settles rather than assumes. `VSync(1)` takes
no wait and writes nothing — it returns `(*(u16*)0x1F801110 - snapshot) & 0xFFFF`. In `0x8008ADA4`
the exit sample is loaded at `0x8008B260` and immediately overwritten by the entry sample at
`0x8008B264`, which is itself immediately overwritten by `lw $v0, 0x24($s4)`. In `0x8008BB48` the
exit sample is overwritten the same way at `0x8008C334`, and the entry sample survives only as the
return value, which the scene walk `0x8007976C` discards. The native owners therefore reproduce the
sample from the framework's own deterministic `Timing::hSyncCounter()` — the same source
`display_frame.cpp` already uses — and no guest clock is returned and no wait is dispatched.

The working tree owns 25 sites, leaving 26 retail call sites. That number is inventory, not a
completion claim. The fatal `0x800320EC` trap remains the runtime falsifier; it may not be replaced
with a counter or wait. The next serialized product trace must name the first residual reachable
owner. Static analysis alone cannot decide an indirect BOOT callback-table selection.

The first rebuilt product run reached the exact fatal trap from resident return address `0x80048700`
before the FrameDriver began. Ghidra and emitted retail code agree that `0x800486DC` disables automatic
pad clearing, waits through `VSync(0)`, enters the memory-card critical section, and installs the BIOS
and vector state. Native boot is single-threaded before `FrameLoopShell` starts, so no guest frame can
race that setup. `memory_card_startup.cpp` preserves every surrounding call, branch, stack/register
transition, and instruction tick while removing the ownerless wait; it does not return a VSync value.

The next bounded product run validated that owner by progressing beyond `0x800486F8`, then trapped
at `VSync(-1)` from `ra=0x8003E6EC` during `CD_init`. Its live ancestry was
`0x80012E90 -> 0x800279A4 -> 0x80034AFC -> 0x80034B8C -> 0x8003F29C -> 0x8003EBF8 -> 0x8003E6B0`.
Ghidra and emitted code show that `0x8003E6B0` and `0x8003EBF8` use four VSync queries only as
timeouts around an interrupt-driven CD controller. The PC model has no controller and performs CD
commands, synchronization, and ISO lookup synchronously. `cd_startup.cpp` therefore owns the
controller-ready handshake, while the measured libcd entries bind to psxport's existing synchronous
CD implementation. This is a host-completion result backed by actual native work, not a fabricated
guest clock.

The following bounded run had no VSync violation or timeout, proving those new chokepoints were
active. It instead timed out in `0x800349AC -> 0x8003584C`: Crash Bash's drive-readiness query waits
for a `GetTN` status packet whose first byte is `2`, while the generic no-controller command correctly
returns no invented status bytes. `cdDriveReadyOwned` checks `disc_open` and the parsed CHD TOC
directly, returns ready only from that native evidence, and owns the two retry-branch VSync sites in
`0x800349AC`.

That run validated real TOC readiness, opened the CHD, printed `load file start`, and then hit the
fatal trap at `VSync(-1)`, `ra=0x80034858`, through
`0x800134FC -> 0x80027790 -> 0x8003470C`. The reached function is the guest's interrupt-driven file
read starter: it records a VSync timeout clock, starts asynchronous sector delivery, and returns an
in-flight result. `cd_file_read.cpp` now owns the higher measured `0x80027790` contract and copies
the requested descriptor-relative 2048-byte sectors from the real CHD before returning success. It
clears the same guest active flag and retains the full generated async body; no VSync value or
completion flag is fabricated. The next strict product gate passed: both real-sector loads completed,
then `empty prims` and the measured MENU observer were reached with no timeout, fatal, recomp miss,
or guest-VSync violation. The nested
`0x8003470C`/`0x800348A8` sites remain in the residual inventory because they have other static
callers and must still trap if a different unowned path reaches them.

A separate direct 120-native-frame product run continued beyond that boot gate. Exact child PID
`2579916` exited by itself with code 139 after reaching the fatal trap at `VSync(-1)`,
`ra=0x8002D9E4`, through
`FrameDriver -> 0x80010394 -> BOOT 0x80092BA0 -> 0x8001E610 -> MENU 0x800B5218 -> BOOT 0x8008E5BC -> 0x8002D4F4`.
The first two PRESENT captures were 960x720 and exactly 0/691200 non-black pixels; frames 30, 60,
and 119 were not reached. Ghidra and emitted code identify `0x8002D4F4` as a 20-state disc/license
startup sequence with six `VSync(-1)` delay states. Visual inspection falsified the initial reading
of state 16: `0x8002E0F0` draws the red copy-protection warning and calls BIOS `B0:38 exit`; the
authentic-disc path instead completes Pause in state 18 and returns to idle state 0. The corrected
native owner validates the measured one-track geometry, executable/SYSTEM.CNF/DAT extents, and exact
SYSTEM.CNF, then records state 0. Its Clang build and static ownership gates pass, but it still awaits
a serialized product run.

The GPU timeout roots are a distinct case: psxport's native GPU completes submission synchronously,
so `gpu_timeout.cpp` records an idle timeout state and continues the successful-ready transfer path.
It does not sample HSync or VBlank, and it keeps the generated bodies registered as the oracle/A-B
path.
