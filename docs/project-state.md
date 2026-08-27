# Project state

| ID | Capability / observable outcome | State | Dependencies | Goals |
|---|---|---|---|---|
| S001 | The selected USA disc, executable, BOOT module, and MENU module are reproducibly identified and derived | verified | — | G001 |
| S002 | The retail boot reaches a guest-visible first frame with faithful drive timing | partial | S001 | G001 |
| S003 | Resident, BOOT, and MENU code form a reproducible executable recompilation substrate | verified | S001 | G001 |
| S004 | Crash Bash graphics are produced natively from decoded game state | partial | S002, S003 | G001, G002, G003 |
| S005 | The native camera supports wider aspect ratios without changing vertical framing | missing | S004 | G002 |
| S006 | Native camera and world transforms render between simulation ticks | missing | S004 | G003 |
| S007 | Deterministic checks compare the port with independent retail behavior at proven boundaries | partial | S001 | G001, G002, G003 |

## Current focus

S004 is the current focus. The first source-owned producer now copies the title-composed affine and
projection state at `0x8001965C`, decodes fixed-frame Gouraud triangle strips plus their descriptor-run
UV, texture-page, and CLUT state from model source records, stores them in `SceneSnapshotHistory`, and
submits through the native render queue under producer key `0x80019F1C`. It retains every generated
super and reads no GTE result, ordering table, or GP0 packet. At real-disc frame 255 all 71 accepted
models carry copied transforms; 1,542 faces are captured, including 382 textured faces, and 1,010
survive winding/depth rejection. The 4:3 native capture is non-black at 627672/691200 pixels and the
before/after image visibly gains three sampled gradient squares plus particle points; the serialized
PSX reference is 668153/691200. This proves a textured source path, not 4:3 picture correctness: at
frame 300 the native producer still renders only central Eurocom-letter fragments while the PSX path
renders the complete logo. The next unit is source-owned ordering/coverage parity for `0x80019F1C`,
not widescreen or interpolation.

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

The port now reaches the Crash Bash TITLE SCREEN. Two defects were resolved to get there. Issue 0014:
the emitter was binding a direct call into the nested MENU range to the BOOT module's own stale body,
so the port executed the wrong module's code once MENU loaded; direct calls into a narrower
overlapping module's range now route through the resident-overlay router. Issue 0012: the two BOOT
object callbacks `0x8008ADA4` and `0x8008BB48` are natively owned in `boot_object_callbacks.cpp`,
taking guest-VSync ownership to 25 of 51 sites with 26 guarded residuals.

The 600-frame product run now exits 0 with no guest-VSync violation, recompilation miss, watchdog
stall or fatal trap, and the guest submits 737,668 primitives (previously 2). Rendered through the
diagnostic PSX path as an oracle it presents the title screen at 92.9%/98.9%/83.6% non-black, which
proves the simulation is producing a real picture. The SHIPPING native path still presents black
because no native producer exists — that is S004, below.

Gap: Issue 0009 remains open. Issues 0012 and 0014 are resolved. A non-black frame on the SHIPPING
native path still requires S004's native producers. The strict MENU
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

Partial capability: Crash Bash now has one native producer that reads decoded game camera/object/
material state and submits fixed-frame untextured and textured geometry. `tools/inventory_render_anchors.py` binds its answer to the exact generated
input/output hashes and psxport commit. Both previous pin `17981527` (1,724 functions) and substrate
baseline `99a42aa3` (2,005 functions) contain 31 projection anchors and 17 camera-control anchors,
including the stable resident `0x80015780 -> 0x8001CD04 -> {0x800193A8, 0x8001AF2C}` chain, although
three individual anchor addresses differ. `CrashBashRuntime` now declares the intended
`interpolatedNative` product policy, but policy does not create a producer; runtime attribution and
state semantics remain unproven.
`tools/verify_render_anchor_reach.py` passes 8/8 known/unknown/zero/empty/fatal controls and NOW HAS
ITS FIRST REAL POSITIVE TRACE: once the BOOT object callbacks were owned, a 600-frame run produced
8 histograms and 765,553 projection calls attributed to exactly the two anchors the static inventory
predicted, `0x800193A8` and `0x8001AF2C`, reached from `0x80019DAC` and `0x80019D7C`. That proves
runtime projection ancestry only; camera semantics, scene state, primitive ownership, 4:3 parity,
widescreen and interpolation all remain unproven. See issue 0011 and
`docs/findings/render-anchor-inventory.md`.

The clean `0d4712f` product A/B established the original gap directly: 600/600 native-owned frames complete on
both paths with no guest-VSync violation, but the shipping native captures are entirely black while
the PSX diagnostic captures visibly show Eurocom and the Crash Bash title. Semantic decompilation
now traces the live anchors backward through model decoder `0x80019A60` to pre-GTE object submitters
`0x80019F1C` / `0x8001DD50`, transform composer `0x8001965C`, and source-vertex decode/interpolation
`0x8001C1E0` / `0x8001C0F0`. A working candidate captures those accepted object/model/transform
inputs into two title-owned frame snapshots while retaining the generated supers. A 350-frame
real-disc run reaches the standard capture 17,004 times and returns cleanly; the history rotation
selftest passes, and a second run reports 71 accepted model draws in frame 255. The current producer
extends that boundary through source fixed-frame topology/material/UV decode and 4:3 native
submission: frame 255 captures 1,542 faces, of which 382 are textured, submits 1,010, and produces a
visibly non-black 90.81% native picture versus the 96.67% PSX reference. A retained untextured capture
compared with the new capture visibly proves the textured faces contribute sampled gradient squares
and particles. Frame-300 inspection nevertheless shows incomplete logo geometry, so source-owned OT
sort/coverage parity and remaining producer families are still missing.

### S005 — Widescreen camera

Missing capability: no PC-owned Crash Bash camera/projection producer exists. The nearest measurable
gate is runtime proof of the active projection-state owner, then 4:3 native parity, followed by wider
horizontal view with unchanged vertical framing. GTE/OT/GP0 output cannot be the source.

### S006 — Interpolated presentation

Missing capability: the PC-owned history now retains consecutive copied camera/world transform and
fixed-face snapshots, but no presentation-time render consumes an interpolated pair. The nearest
measurable gate is unchanged simulation with interpolation disabled, then an alpha-0.5 native render
between verified snapshots without guest-RAM mutation.

### S007 — Differential verification

The independent Beetle oracle and title-local verifiers establish interrupt ordering, module loads,
command phase progress, and the still-divergent guest completion result. Static render inventory now
has controlled positive and negative fixtures.

Gap: full-memory lockstep, a passing clean pinned CDC/completion run, one presented game frame,
first-frame runtime submitter attribution, and native 4:3 image parity are not yet demonstrated. The
old exact-pin watchdog refusal and the corrected exact-pin `784e5212` MENU gate are valid evidence for
their declared boundaries; neither satisfies any of those later gates.
