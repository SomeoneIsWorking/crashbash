# Project state

## Comparison baseline

The baseline is the unmodified USA PlayStation release of *Crash Bash* running on original hardware
or through a PS1 emulator, with retail game modes, a 4:3 camera, 30 Hz presentation, and console CPU
execution. The intended product authenticates the user's game image, executes selected behavior in
native title owners, dynamically translates all remaining MIPS through psxport's pinned Lightrec
revision, widens the camera, and adds 60 Hz interpolated presentation without accelerating simulation.

## Current focus

**S003** — Consume psxport's per-`Core`, pinned-Lightrec executor and prove that the gameplay product
contains no interpreter. The former static product has already been deleted and is not a bridge,
fallback, or oracle.

## Capability inventory

| ID | Capability / observable outcome | State | Dependencies | Goals |
| --- | --- | --- | --- | --- |
| S001 | The selected USA disc, executable, and measured `CRASHBSH.DAT` modules are reproducibly authenticated and provisioned | verified | — | G001 |
| S002 | The retail boot and loaded-image sequence have a recorded first-frame and menu frontier to re-establish through the dynarec | partial | S001, S003 | G001 |
| S003 | The gameplay product executes every non-native guest path through psxport's pinned Lightrec dynarec and contains no interpreter | missing | S001, shared psxport executor | G001 |
| S004 | Crash Bash graphics are produced natively from decoded game state and look correct across representative content | partial | S002, S015 | G001, G002, G003 |
| S005 | The native camera supports wider aspect ratios without changing vertical framing | partial | S004 | G002 |
| S006 | Native camera and world transforms render between simulation ticks | partial | S004 | G003 |
| S007 | Deterministic diagnostics compare reached hybrid-product boundaries with independent retail behavior and prove both answers | partial | S001, S003 | G001, G002, G003 |
| S008 | The retail game modes are reachable and playable end to end on the hybrid product | partial | S002, S003, S004, S015 | G001 |
| S009 | The Crashball gameplay scenario reaches a live match and accepts player control | verified | recorded behavior; dynarec requalification in S016 | G001 |
| S010 | Battle Mode Crate Crush reaches a live match and accepts player control | verified | recorded behavior; dynarec requalification in S016 | G001 |
| S011 | Tournament Mode reaches its first live Crate Crush match and accepts player control | verified | recorded behavior; dynarec requalification in S016 | G001 |
| S012 | Polar Push reaches a visually correct, controllable live match | verified | recorded behavior; dynarec requalification in S016 | G001 |
| S013 | The remaining retail modes are reachable and playable | missing | S008 | G001 |
| S014 | Retail music and sound effects play at the correct rate without premature truncation | partial | S003 | G001 |
| S015 | All 27 native overrides install by runtime image identity and all 15 original-body calls execute through the dynarec | partial | S003 | G001 |
| S016 | Representative interactive gameplay passes on the native/dynarec product | missing | S003, S004, S005, S006, S007, S008, S014, S015 | G001, G002, G003 |
| S017 | Every static product path is deleted before dynarec implementation and mechanically excluded | verified | — | G001 |
| S018 | Hosted CI truthfully covers applicable Linux, Windows, macOS, and Android product boundaries | partial | S003 | G001 |

The verified S009-S012 entries describe durable reached behavior and replay inputs, not dynamic-engine
completion. They become dynarec conformance evidence only after those scenarios run through the hybrid
gameplay product with nonzero Lightrec execution and the no-interpreter product audit.

## Capability details

### S001 — Authenticated retail inputs

Evidence: The USA disc resolves through `SYSTEM.CNF` to the exact `SCUS_945.70` identity, and tracked
manifests identify the resident executable plus all measured `CRASHBSH.DAT` images without tracking
copyrighted bytes.

### S002 — Boot and loaded-image frontier

Evidence: Recorded runs reach the first presented frame, MENU `0x800B5244`, the Cross-selected
DAT28136 registration/update boundary, and later interactive scenarios with the measured native boot,
device, and frame owners active.

Gap: Re-establish the complete reached sequence through pinned Lightrec with nonzero dynamic execution,
image-correct invalidation, bounded exits, and no interpreter in the gameplay product.

### S003 — Pinned-Lightrec gameplay executor

Missing capability: Integrate psxport's maintained, pinned Lightrec revision as the sole gameplay
executor for every non-native guest instruction, with product-link and selector evidence proving the
interpreter is absent and no fallback exists.

### S004 — Native graphics coverage

Evidence: Native model, sprite, authored-screen, ordering, camera, and scene-snapshot owners render the
measured startup, menu, Crashball, Crate Crush, Tournament, and Polar Push content from decoded game
state rather than GTE/OT/GP0/framebuffer output.

Gap: Requalify these owners across representative dynarec gameplay and complete the remaining visible
4:3, wider-mode, effect, UI, and transition coverage.

### S005 — Widescreen camera

Evidence: The native camera shows additional horizontal Crashball coverage while preserving vertical
framing, and authored 4:3 presentation compositions remain centered.

Gap: Verify projection, viewport, scissor, HUD intent, and proven culling ownership through the other
representative retail modes on the hybrid product.

### S006 — Interpolated presentation

Evidence: The native path rebuilds midpoint model geometry from consecutive immutable title snapshots
without changing the retail simulation cadence or mutating guest RAM.

Gap: Complete and verify interpolation for all reached world, effect, UI, and transition families on
representative hybrid gameplay.

### S007 — Independent diagnostics

Evidence: Existing emulator comparisons and title-local judges cover interrupt ordering, module loads,
command phases, input delivery, menu transition, native ownership, and graphics attribution with
controlled opposite answers.

Gap: Migrate every still-useful judge to the shipping native/dynarec boundary and add product-link,
selector, translated-block, override/original-call, invalidation, and representative-gameplay coverage.

### S008 — End-to-end retail mode coverage

Evidence: Durable bounded scenarios cover live, controllable Crashball, Battle Crate Crush, Tournament
Crate Crush, and Polar Push behavior.

Gap: Re-run that frontier through pinned Lightrec and cover every remaining retail mode family.

### S009 — Crashball scenario

Evidence: The tracked 3,740-frame replay reaches a live DAT28241 Crashball match and visibly moves the
player ship left and right.

### S010 — Battle Crate Crush scenario

Evidence: The tracked 15,401-frame replay crosses the objective, controls, and special-items pages,
enters a live Battle Mode Crate Crush match, and moves the player in both directions.

### S011 — Tournament Crate Crush scenario

Evidence: The tracked 7,910-frame replay reaches Tournament Mode's first live Crate Crush match and
moves the player in both directions.

### S012 — Polar Push scenario

Evidence: The tracked 17,682-frame replay reaches a complete Polar Push match with the four-player HUD,
arena, ships, balls, and controllable player movement.

### S013 — Remaining modes

Missing capability: Add durable, controllable hybrid-product scenarios for every retail mode not
covered by the four retained gameplay routes.

### S014 — Audio playback

Evidence: The measured host sink sustained 44,097 samples/s against the 44,100 Hz target with zero
dropped fields, while the bounded pre/post WAV remained byte-identical.

Gap: Requalify hardware listening, longer gameplay, and every title audio path through pinned-Lightrec
gameplay.

### S015 — Runtime overrides and original calls

Evidence: the surviving title sources contain 27 registrations through the single image-qualified
`runtime::registerNativeOverride` boundary and all 15 former generated-body calls now enter the single
scoped `runtime::callOriginal` boundary. `tools/verify_native_ownership.py` reports both denominators
and its test suite proves forbidden old paths are detected.

Gap: psxport must connect Lightrec and expose loaded-image lifecycle binding; then implement the thin
adapter over its per-Core API and prove registration, enabled/disabled behavior, recursion suppression,
ABI/state, and cache invalidation at runtime.

### S016 — Representative gameplay

Missing capability: Pass representative interactive gameplay with correct rendering, input, audio,
timing, devices, and per-host frame time on the interpreter-free native/dynarec product.

### S017 — Break-first static-path removal

Evidence: the tracked offline emitter integration, seed file, generated registry installer, and
static-only verifiers are deleted. The ignored generated corpus, prior static build tree, and retained
static product binaries are absent. `tools/source_policy.py` rejects the old files, generated directory,
static dispatch markers, and any change to the 27-registration/15-original-call source boundary.

### S018 — Platform CI coverage

Partial capability: `.github/workflows/ci.yml` runs the asset-free launcher, provisioning, and source
policy tests on one Linux host with full history, read-only permissions, pinned actions, and an
explicit timeout. This is repository-policy coverage, not evidence for a packaged Linux product.

| Platform | Applicability | Current CI evidence and exact gap |
| --- | --- | --- |
| Linux x86-64 | applicable desktop target | Source/launcher policy is covered; CI does not yet configure, compile, lint, test, or package the native/dynarec product because S003 is missing. |
| Windows x86-64 | applicable portable-PC target | Missing: no supported Windows native build, runtime test, first-run setup, or package boundary exists. |
| macOS arm64 | applicable portable-PC target | Missing: no Apple-Silicon native build, runtime test, first-run setup, or application package exists. |
| Android arm64 | applicable mobile target | Android metadata and setup sources exist, but the Lightrec native runtime, shared `android-port` build path, Gradle/NDK APK build, and install/runtime test are missing. |

Gap: the former green Android metadata/selftest job was removed because it did not build or install an
APK. Add Android and desktop jobs only when they drive the actual redistributable platform boundary
without game assets.

## Dynamic migration acceptance

The first Crash Bash dynamic milestone must prove all of the following together:

- the exact authenticated resident and loaded images execute through the pinned psxport/Lightrec
  integration with nonzero translated-block execution;
- product link and configuration inspection proves that no interpreter is linked, selectable, or
  reachable as a fallback;
- all 27 native override installations are keyed by complete runtime image identity and address;
- all 15 former generated-body calls use a scoped original call that suppresses only the current
  override, enters the original guest body through Lightrec, and returns with correct guest state;
- loaded-image replacement at the shared `0x800B32B4` slot invalidates affected translated blocks and
  cannot reuse an override or block under the wrong image identity;
- bounded executor exits preserve VSync/frame, interrupt, exception, host-work, and thread-exit
  ownership without unwinding through translated host frames;
- the existing independently controlled boot, Cross-menu, Crashball, Battle Crate Crush, Tournament
  Crate Crush, and Polar Push scenarios reach their recorded boundaries without a guest-VSync
  violation, wrong-image dispatch, or missing native owner.

Boot, logos, menus, and a first frame are checkpoints only. S016 requires representative interactive
gameplay with observable movement, correct rendering, audio/timing coverage, and declared frame-time
evidence on each released host architecture. The static execution path is already absent and cannot be
used to obtain new evidence; comparison comes from the independent emulator, binary analysis, or a
separately built test target.

## Retained verified title facts

These facts survive the execution-engine migration because they describe the retail binary, user-visible
behavior, or native subsystem contracts rather than the retired translation method.

### Authenticated images and overlay identity

- USA `SYSTEM.CNF` boots `SCUS_945.70`. The PS-X EXE has SHA-256
  `fd5727a18feb2a2d5a6359a55966f0266284d1e50f64ee9b8a127a97091bd516`, entry
  `0x8002E7B0`, load address `0x80010000`, and text size `0x69000`.
- Real CDC/DMA traces place BOOT at LBA 35799. MENU, DAT28272, DAT28241, DAT28136, DAT28382, and
  DAT22510 are authenticated alternatives in the reused nested load region beginning at
  `0x800B32B4`; their tracked manifests remain the identity authority.
- Polar Push DAT22510 is `CRASHBSH.DAT + 0x02B81000`, size `0x23000`, SHA-256
  `fb8ee41f2c19a9c419e7b4240de82237fca09efee98bdfec63b7ec430a18457f`, loaded at
  `0x800B32B4`. Runtime image generation must distinguish it from every other occupant.
- DAT28382 installs initializer `0x800BB370`. DAT22510 uses callback `0x800CBA64`. The measured
  Cross path registers DAT28136 at `0x800B4E1C`, replaces application callback `0x80093038` with
  `0x800B4694`, and then executes that update.
- DAT28136 is the 42-sector image at `CRASHBSH.DAT + 0x0367E000`. DAT28241 is the 31-sector image
  from LBA 28241. DAT28272 is the 38-sector image from LBA 28272; its code registers a behavior vtable
  at `0x8005AA70`, while its name table identifies animation states rather than a second menu phase.

### Boot, device, and frame ownership

- ResetCallback `0x80031A80` calls setjmp `0x8003ACEC` and resumes at `0x80031AE8`; the independent
  oracle observes `0x80031AE8` (`v0=1`, `sp=0x80068B14`) to `0x80031B58` with the same stack and
  `ra=0x80031AF8`.
- The finite native boot owner begins from the measured one-shot `0x8002718C` / `0x80010158` prefix;
  repeating process work remains owned by the frame driver.
- The IRQ2 callback is `0x8003F5F0`, draining through `0x8003E14C`. The VBlank/SIO chain registers
  class 2 element `0x8006D984`; verifier `0x8003B1BC` tests I_STAT bit 0 and handler `0x8003B224`
  drives SIO0, per-byte I_STAT bit 7, and timer 2.
- Measured native ownership boundaries include memory-card startup `0x800486DC`, libcd command/sync
  `0x8003EBF8 -> 0x8003E6B0`, TOC readiness `0x800349AC -> 0x8003584C`, file-read start
  `0x80027790 -> 0x8003470C`, controller handshake `0x80034B8C`, and disc/license state machine
  `0x8002D4F4`. BOOT object callbacks `0x8008ADA4` and `0x8008BB48` are native frame owners.
- The card-event callback is `0x8004718C`; retail code constructs its address at `0x80047280` rather
  than storing a discoverable pointer. The libmcrd device-table walk at `0x8004799C` returns zero for
  both “no such device” and “request started,” so publishing the `bu` device and its completion event
  are independently required.
- The disc owner performs real CHD reads and may complete them synchronously, but it must not fake
  completion, manufacture a guest VSync clock, or weaken frame/watchdog ownership. The independent
  asynchronous oracle remains diagnostic evidence for the 189-sector completion sequence.

### Input and representative flow scenarios

- The pad chain changes packet `0x80077FBC` from `41 5A FF FF` to `41 5A F7 FF`, parsed P1
  `0x80063A92` from `FFFF` to `FFF7`, and active-high P1 `0x8005133C` from 0 to 8; P2
  `0x80051394` remains 0. Direct-buffer injection is not an accepted path.
- The active MENU table `0x800B8E28` updates through `0x800B3CA8`. Rising-edge input
  `0x80051380` accepts Cross `0x4000` at `0x800B3D88-0x800B3D8C` and schedules table
  `0x800B8E50` through `0x8009F8A8`. START is not a substitute for this transition.
- Portal-selection byte `0x8005A677` changes from `0xFF` to `0x00` before the DAT28241 load. Guest
  state `0x8004E0B8` and application mode `0x80078C90` remain the measured transition witnesses.
- `replays/flow/crashball-control.pad` contains 3,740 active-low masks; frames 3560-3619 hold Left
  and 3620-3739 hold Right in a live DAT28241 Crashball match.
- The recorded 15,401-frame Battle Mode replay reaches a live Crate Crush match; the 7,910-frame
  Tournament replay reaches its first Crate Crush match; the 17,682-frame Polar Push replay reaches
  a complete, controllable live match. These are the minimum retained scenarios for dynarec
  requalification, not claims that every retail mode is covered.

### Native graphics and presentation contracts

- Runtime projection anchors are `0x800193A8` and `0x8001AF2C`; model/object ancestry includes
  `0x80019A60`, submitters `0x80019F1C`/`0x8001DD50`, transform composition `0x8001965C`,
  source decode `0x8001C1E0`/`0x8001C0F0`, and prefill `0x80017EE8`.
- The stable resident ancestry is `0x80015780 -> 0x8001CD04 -> {0x800193A8, 0x8001AF2C}`;
  runtime callers `0x80019DAC` and `0x80019D7C` independently reached the two projection anchors.
- `0x8009440C` proves projection H comes from `camera + 0x18`; `projectionGlobals + 4` is a separate
  horizontal OFX scale. `0x80018B08` is viewport/camera/render-list setup, not a drawable boundary.
- Native sprite boundaries `0x8002992C`, `0x80029D28`, and the authored-screen branch of
  `0x8001A0D8` consume game-owned descriptor, position, color, texture, scale, fade, and ordering
  data. They do not consume OT, GP0, VRAM, or framebuffer output as product inputs.
- `0x800274FC` and `0x800276C4` are the two heap-pool allocators. Their live base/end ranges, the
  4,096-entry ordering tables, frame-wide authored ordering, AVSZ3/OTZ rejection, and immutable
  scene snapshots remain native contracts.
- Retail initialization at `0x80033494` publishes ZSF3 341. Packet `0x800C5394` identifies object
  `0x800A0C74`, frame `0x200B`, face 261, material `0x02FB`; AVSZ3 yields OTZ 1511/sort 1755. The
  cross-object order witness is dark node `0x800C2FF4` versus red node `0x800C8D84`/object
  `0x801E18B0`, for which frame-wide authored order preserves the retail winner.
- Widescreen currently works in measured Crashball and startup scenes but needs broader mode coverage.
  Interpolation rebuilds midpoint model geometry from consecutive immutable snapshots and remains
  partial until its source families and gameplay coverage are complete.

Detailed provenance remains in `docs/findings/`, `docs/issues/`, and `docs/info/`. Those records are
evidence inputs, not permission to restore a generated gameplay path.
