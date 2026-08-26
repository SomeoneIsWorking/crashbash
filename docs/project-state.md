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

S002 is the current focus. The controller-level response-edge fix is landed and candidate-tested. A
serialized product run against exact recorded pin `784e5212` now
completes both module loads, prints `empty prims`, and reaches the measured MENU entry `0x800B5244`
from resident caller `0x8001E7C0` without a watchdog stall. It still does not contain the CDC
diagnostics needed to judge phase progress or the retail guest-visible completion result `1 -> 0`,
and it does not demonstrate a presented game frame.

## Capability details

### S001 — Reproducible retail inputs

Evidence: `tools/provision.py`, `tools/loaded_module.py`, and `tests/test_provision.py` verify the USA
`SCUS_945.70` identity plus the full DAT, BOOT, and MENU payload measurements recorded under
`titles/crashbash/`. Provisioning passes 11/11 controlled positive/refusal/mutation checks; no game
bytes are tracked.

### S002 — Faithful boot to first frame

The port preserves the independent Beetle interrupt-exit prefix, executes both measured loaded
modules, and the landed CDC phase candidate crosses the former GetTN boundary through later reads.
On former exact pin `99a42aa3`, the clean product completed both loads and printed `empty prims`,
but an intentional bootstrap black clear incorrectly marked the main presenter ready and the process
exited 134 on the steady `[watchdog] STUCK` timeout. Exact recorded pin `784e5212` fixes that phase
ownership with typed main-frame versus transition completion. Its serialized product gate
observes the deterministic order `2/2 loads -> empty prims -> MENU 0x800B5244 from ra=0x8001E7C0`
without STUCK, fatal output, or a recompilation miss. The framework passes 107/107 CTests and the
consumer passes 11/11 CTests against that clean pin.

Gap: The corrected boot log is not a CDC trace. Issues 0007 and 0009 remain open until the process-driving
`tools/verify_cdc_phase_progress.py --port ... --executable ... --timeout 30` gate reaches LBA 17655,
the guest-visible completion result `1 -> 0` is captured, and the post-MENU no-present cause is
identified. Saved CDC logs require `--trace PATH`; positional logs are rejected.

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
