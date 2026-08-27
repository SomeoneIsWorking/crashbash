# Project state

| ID | Capability / observable outcome | State | Dependencies | Goals |
|---|---|---|---|---|
| S001 | The selected USA disc, executable, BOOT module, and MENU module are reproducibly identified and derived | verified | — | G001 |
| S002 | The retail boot reaches a guest-visible first frame with faithful drive timing | partial | S001 | G001 |
| S003 | Resident, BOOT, and MENU code form a reproducible executable recompilation substrate | verified | S001 | G001 |
| S004 | Crash Bash graphics are produced natively from decoded game state | missing | S002, S003 | G001, G002, G003 |
| S005 | The native camera supports wider aspect ratios without changing vertical framing | missing | S004 | G002 |
| S006 | Native camera and world transforms render between simulation ticks | missing | S004 | G003 |
| S007 | Deterministic checks compare the port with independent retail behavior at proven boundaries | partial | S001 | G001, G002, G003 |

## Current focus

S002 is the current focus. The current strict serialized product gate completes both module loads,
prints `empty prims`, and reaches measured MENU entry `0x800B5244` from resident caller
`0x8001E7C0` with no guest-VSync violation, timeout, fatal, watchdog, or recompilation miss.
The current working tree replaces the retail lifetime process runner with a finite Crash Bash
FrameDriver and owns display function `0x800272AC` plus its nested allocator without calling guest
VSync. The first bounded post-change product run then reached the mandatory fatal trap during
boot-time memory-card BIOS/vector startup at `0x800486DC`, from `ra=0x80048700` and ancestry
`0x80027F00 -> 0x8002C894 -> 0x8003ABAC`. A new title-local owner preserves that setup without the
ownerless `VSync(0)`. The next bounded run validated it and reached a distinct `VSync(-1)` timeout
query from `ra=0x8003E6EC` during `CD_init`. The rebuilt candidate now owns the controller-ready
handshake and routes the measured libcd command, sync, and ISO lookup entries through psxport's
synchronous native CD implementation. That run had no VSync violation or timeout, but exposed a
`GetTN` readiness loop at `0x800349AC -> 0x8003584C`: the no-controller command has no invented status
packet. The rebuilt title owner now derives readiness from a real parsed CHD TOC. The next run
validated readiness, opened the real CHD, and
reached `load file start`, then trapped at the guest async read starter through
`0x800134FC -> 0x80027790 -> 0x8003470C`. The rebuilt candidate now owns the higher file-read
contract, copying every requested real sector into guest RAM before returning; the strict pass
validated both loads and MENU. A separate direct 120-frame run then exited 139 at the fatal VSync
trap from `ra=0x8002D9E4` under disc/license state machine `0x8002D4F4`; PRESENT frames 1 and 2 were
both 960x720 and 0% non-black. The first owner candidate then produced only the red copy-protection
failure screen: visual inspection and BIOS call `B0:38 exit` proved state 16 / `0x8002E0F0` is the
failure path. The corrected title owner binds the runtime disc to the measured SCUS-94570 layout and
records the authentic-disc idle state 0 without a guest clock. Its serialized run accepted that
identity and removed the red failure screen, then watchdoged in stock libmcrd directory enumeration:
psxport's synchronous firstfile/nextfile returned an empty result without delivering the HwCARD
completion that `0x800476EC` waits on. That shared fix alone did NOT clear the trap: a
`PSXPORT_DEBUG=card,ev` trace proved `firstfile` was never reached, because Crash Bash never called
`card_overrides_init` and so the `"bu"` BIOS device was absent from the kernel device table that its
libmcrd walks itself (issue 0013). With the title wiring added, the direct 120-frame run now exits 0,
completes 120/120 frames, and returns from the native crt0 with no watchdog stall, fatal trap,
recompilation miss, or guest `VSync: timeout`, and the strict serialized MENU gate passes on the same
build. Static VSync ownership remains 21 of 51 sites with 30 mapped residuals and no further guest
VSync root was reached. The sole remaining S002 gap is presentation content: every captured PRESENT
is still 0% non-black, which is the native-graphics gap (S004), not a boot or timing defect.

## Capability details

### S001 — Reproducible retail inputs

Evidence: `tools/provision.py`, `tools/loaded_module.py`, and `tests/test_provision.py` verify the USA
`SCUS_945.70` identity plus the full DAT, BOOT, and MENU payload measurements recorded under
`titles/crashbash/`. Provisioning passes 11/11 controlled positive/refusal/mutation checks; no game
bytes are tracked.

### S002 — Faithful boot to first frame

The port preserves the independent Beetle interrupt-exit prefix, executes both measured loaded
modules, and the landed CDC phase candidate crosses the former GetTN boundary through later reads.
On former exact pin `99a42aa3`, the product completed both loads and printed `empty prims`,
but an intentional bootstrap black clear incorrectly marked the main presenter ready and the process
exited 134 on the steady `[watchdog] STUCK` timeout. Exact recorded pin `784e5212` fixes that phase
ownership with typed main-frame versus transition completion. Its serialized trace observes the
deterministic order `2/2 loads -> empty prims -> MENU 0x800B5244 from ra=0x8001E7C0` without STUCK,
fatal output, or a recompilation miss, but it is not a passing product gate: seven guest
`VSync: timeout` lines were previously accepted. `tools/verify_boot.py` now rejects them and passes
13/13 controlled answers. The current native-ownership candidate has a typed fatal VSync trap,
finite boot/process ownership, and retained display/GPU overrides. Its first serialized launch
proved the trap and named boot-time memory-card setup `0x800486DC` as the next owner before MENU. A
second serialized launch validated that owner, then identified Sony libcd's controller/IRQ timeout
path through `0x8003E6B0` and `0x8003EBF8`. The current Clang build retains both generated bodies,
owns the controller handshake at `0x80034B8C`, and uses the framework's synchronous CD command, sync,
and ISO lookup owners. A third serialized launch had zero VSync violations/timeouts and exposed the
distinct `GetTN` status hang at `0x800349AC -> 0x8003584C`. The rebuilt candidate owns that top-down
readiness contract from the actual CHD TOC. A fourth run validated it and reached the distinct guest
async file-read path at `0x80027790 -> 0x8003470C`. The rebuilt candidate performs that read
synchronously from the real CHD, with the generated body retained. A fifth serialized launch passes
the strict `2/2 loads -> empty prims -> MENU` gate without a VSync violation. The direct 120-frame
run then names disc/license state machine `0x8002D4F4` as the next fatal root and captures two fully
black presents. A first synchronous owner rendered only the red copy-protection failure screen,
falsifying its state-16 interpretation. The corrected owner follows measured state-18-to-0 success;
its direct launch accepted the authentic disc and removed that failure path, then exposed an
independent missing HwCARD completion after an empty firstfile result. A focused shared psxport
regression now proves firstfile and nextfile each preserve zero and invoke the interrupt-mode
completion callback exactly once; another serialized launch remains required.

The memory-card boundary is now resolved. Issue 0013 records the root cause: the framework entry that
publishes the card as a BIOS device was never called by this title, and libmcrd's own device-table
walk at `0x8004799C` returns 0 for "no such device" — the same value it returns for "request
started" — so the caller waited forever on an operation that was never begun. A three-way A/B on the
real disc proves the title wiring and the shared completion event are each necessary and neither is
sufficient alone.

The port now reaches and runs the MENU scene: the application-shell process state, the BOOT-overlay app
mode, and the scene pointer into the loaded MENU image are each measured, with update/present called
every frame. The guest still performs zero projection work there, and a 600-frame run aborts on an
indirect call through an uninitialised function pointer (issue 0014), which is the next boundary.

Gap: Issues 0009, 0012 and 0014 remain open until a real non-black game frame is presented. The strict MENU
gate is green and the 120-frame product run is now clean end to end, but both remain boot/module and
lifecycle boundaries and do not certify picture content.

### S003 — Executable recompilation substrate

Evidence: previous psxport pin `17981527` derived 1,724 generated functions across 941 resident, 696
BOOT, and 87 MENU functions. Substrate baseline `99a42aa3` derives and integrity-checks a distinct
2,005-function substrate across 1,083 resident, 823 BOOT, and 99 MENU functions; the exact-pin Clang
build at current recorded pin `784e5212` retains both overlay bodies and its 11 CTests pass. The
framework change from the substrate baseline is confined to present/watchdog lifecycle ownership;
the generated directory is
ignored and rebuilt from verified external inputs. This proves the recompilation substrate, not the
serialized product runtime.

### S004 — Native graphics producers

Missing capability: no Crash Bash producer reads decoded game camera/object/material state and
submits native geometry. `tools/inventory_render_anchors.py` binds its answer to the exact generated
input/output hashes and psxport commit. Both previous pin `17981527` (1,724 functions) and substrate
baseline `99a42aa3` (2,005 functions) contain 31 projection anchors and 17 camera-control anchors,
including the stable resident `0x80015780 -> 0x8001CD04 -> {0x800193A8, 0x8001AF2C}` chain, although
three individual anchor addresses differ. `CrashBashRuntime` now declares the intended
`interpolatedNative` product policy, but policy does not create a producer; runtime attribution and
state semantics remain unproven.
`tools/verify_render_anchor_reach.py` passes 8/8 known/unknown/zero/empty/fatal controls and correctly
refuses the exact-pin live attempt: the log ends in forbidden `[watchdog] STUCK` and has no completed
50-frame histogram. It therefore has no real positive trace yet. See issue 0011 and
`docs/findings/render-anchor-inventory.md`.

### S005 — Widescreen camera

Missing capability: no PC-owned Crash Bash camera/projection producer exists. The nearest measurable
gate is runtime proof of the active projection-state owner, then 4:3 native parity, followed by wider
horizontal view with unchanged vertical framing. GTE/OT/GP0 output cannot be the source.

### S006 — Interpolated presentation

Missing capability: no PC-owned pair of consecutive Crash Bash camera/world transform snapshots
exists. The nearest measurable gate is unchanged simulation with interpolation disabled, then an
alpha-0.5 native render between verified snapshots without guest-RAM mutation.

### S007 — Differential verification

The independent Beetle oracle and title-local verifiers establish interrupt ordering, module loads,
command phase progress, and the still-divergent guest completion result. Static render inventory now
has controlled positive and negative fixtures.

Gap: full-memory lockstep, a passing clean pinned CDC/completion run, one presented game frame,
first-frame runtime submitter attribution, and native 4:3 image parity are not yet demonstrated. The
old exact-pin watchdog refusal and the corrected exact-pin `784e5212` MENU gate are valid evidence for
their declared boundaries; neither satisfies any of those later gates.
