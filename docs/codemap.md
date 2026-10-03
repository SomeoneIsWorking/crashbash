# Codemap

This map records responsibility and placement only. Intent lives in `docs/project-goals.md`, factual
capability state in `docs/project-state.md`, and evidence order in `docs/re-frontier.md`.

## The rule

Every first-party C++ file lives under `game/<concept>/`, and the directory name IS the concept.
A new owner goes in the directory of the thing it owns, never in a neighbour's, and never in a new
"common" directory. The framework (`external/psxport`, the workspace's live checkout) owns everything
reusable; this title owns Crash Bash's game-flow interpretation of it.

## Directories

| Directory | Namespace | Files | One-line responsibility |
| --- | --- | --- | --- |
| `game/entry/` | — (process entry) | `player_entry.cpp` | `main`: read the executable, install `TitleAdapter`, bind the per-Core devices, enter psxport's native boot/frame loop, close the ledger. Nothing else; it composes, it does not implement. |
| `game/execution/` | `crashbash`, `crashbash::runtime` | `title_adapter.{h,cpp}`, `guest_execution.{h,cpp}`, `image_identity_state.{h,cpp}`, `native_owner_set.{h,cpp}`, `image_content_identity.h`, `executable_identity.h.in`, `module_image_identity.h.in` | The typed psxport boundary: `TitleAdapter` (the `GameRuntime` the framework drives), `GuestExecution` (per-Core title context: override registrations, authenticated image bindings, original calls), `ImageIdentityState` (those bindings in a save state), `NativeOwnerSet` (the complete installation list), and the CMake-derived identity metadata. |
| `game/boot/` | `crashbash` | `crashbash_boot.{h,cpp}`, `boot_object_callbacks.{h,cpp}`, `boot_logo_skip.{h,cpp}`, `memory_card_startup.{h,cpp}`, `resident_image.{h,cpp}`, `boot_image.{h,cpp}`, `menu_image.{h,cpp}`, `authenticated_module_image.{h,cpp}`, `nested_module_image.{h,cpp}` | Everything that runs once at power-on: the finite retail boot prefix, the BOOT overlay's object callbacks and logo controller, BIOS memory-card startup, and the authenticated publication of the resident executable, BOOT and MENU (plus `NestedModuleImage` for the five overlays that reuse MENU's load slot). |
| `game/disc/` | `crashbash` | `cd_startup.{h,cpp}`, `cd_license_startup.{h,cpp}`, `cd_file_read.{h,cpp}`, `loading_card_skip.{h,cpp}` | The disc: the measured SCUS_945.70 startup handshake, licence startup, the synchronous file read that also retires authenticated images per sector write, the load-pump drain, and the retirement of the loading-card presentation at its two measured call sites. |
| `game/frame/` | `crashbash` | `crashbash_frame_driver.{h,cpp}`, `display_frame.{h,cpp}`, `gpu_timeout.{h,cpp}`, `measured_guest_call.h` | The frame turn: `CrashBashFrameDriver` owns one measured input→audio→simulation→render→present order, the single presentation commit, and the per-Core render census state its native overrides write to (`modelTransformCapture`, `materialDiagnostic`, `facePixelDiagnostic`, `packetIdentityDiagnostic`, `transformInputDiagnostic`) plus the Polar Push `polarContactCensus`; `registerDisplayFrameOverride` is the guest's own frame call it is invoked from; `registerGpuTimeoutOverrides` is the synchronous GPU timeout/transfer; `measuredGuestCall` is the one way a native owner calls a guest body. |
| `game/input/` | `crashbash`, `crashbash::input` | `crashbash_input_phase.{h,cpp}`, `crashbash_touch_controls.{h,cpp}` | `InputPhase` (which screen is taking input — the key a `.pad` recording is keyed on) and `CrashBashTouchControls` (the authored landscape touch overlay for Android). |
| `game/render/` | `crashbash::render` | `model_submit_capture`, `model_transform_capture.{h,cpp}`, `model_depth_scale_capture`, `model_recipe_capture`, `model_frame_source`, `sprite_quad_capture`, `sprite_quad_decode`, `sprite_render_list.h`, `render_viewport.{h,cpp}`, `native_model_producer`, `native_sprite_quad_producer`, `model_face_coverage`, `model_packet_identity.{h,cpp}`, `model_material_diagnostic.{h,cpp}`, `model_face_pixel_diagnostic.{h,cpp}`, `model_packet_identity_diagnostic.{h,cpp}`, `model_transform_input_diagnostic.{h,cpp}` | Native picture production: capture decoded game state at the guest's own submit leaves, resolve recipes/animation banks, clip to the viewport the draw was submitted under, and submit natively. `model_packet_identity` is the stateless packet identity/geometry analysis shared by producer and observer; the four `*_diagnostic` owners (`MaterialDiagnostic`, `FacePixelDiagnostic`, `PacketIdentityDiagnostic`, `TransformInputDiagnostic`) and `ModelTransformCapture` hold their frame census as instance state and observe rendered output as evidence — they select no execution path. All are owned per-Core by `CrashBashFrameDriver`. |
| `game/render/widescreen/` | `crashbash::render::ui_anchor`, `crashbash::render::hud_layout` | `ui_anchor.{h,cpp}`, `hud_layout.h` | The one horizontal anchoring policy (`ui_anchor`) and the guest's HUD families with each one's anchor class (`hud_layout`). Presentation only: no guest memory is read or written. |
| `game/render/fps60/` | `crashbash::render` | `scene_snapshot.{h,cpp}`, `interpolated_scene.{h,cpp}` | Immutable per-tick scene snapshots and the 60 fps in-between built from two completed snapshots. Simulation is never advanced and guest state is never read at present time. |
| `game/gameplay/` | `crashbash::polar` | `polar_push_contact.{h,cpp}` | The DAT22510 native owner of Polar Push contact traversal, motion and effects. Frame and presentation ownership, and the per-Core `ContactCensus` tally, stay in `CrashBashFrameDriver`. |
| `game/debug/` | `crashbash::debug` | `dev_arena.{h,cpp}` | The developer `arena` control-channel command. It ARMS a request; the frame driver advances it through the game's own menu flow. No player input reaches it. |
| `game/diagnostics/` | `crashbash::diagnostics` | `run_ledger.{h,cpp}`, `scene_machine.{h,cpp}`, `menu_boundary.{h,cpp}` | `RunLedger` owns the run-end report; `registerSceneMachine` and `registerMenuBoundary` are read-only observers of guest-owned state. |
| `game/title/` | `crashbash::guest` | `crashbash_guest.h` | The recovered SCUS_945.70 guest facts: addresses, scene/scene-record layouts, arena table and menu flow. Facts only; no behavior. |
| `packaging/linux/` | `crashbash::appimage` | `launcher_main.cpp`, `install_media.{h,cpp}`, `user_paths.{h,cpp}` | The AppImage launcher's first-run media setup. `user_paths.cpp` is this product's only environment-read boundary. |
| `platform/android/` | — (Java) | `app/src/main/java/.../CrashBashActivity.java`, `CrashBashMediaImport.java` | Android Activity and SAF media policy. |
| `tools/` | — (Python) | `provision.py`, `source_policy.py`, `verify_*.py`, `probe_*.py`, `psxport_fetch.py`, `verify.py` | Provisioning, the source-boundary gate, and the product/oracle-boundary verifiers. |
| `tests/` | — | `test_*.cpp`, `test_*.py` | The title's unit and selftest set, run by CTest as `crashbash_*`. |

## Who owns it

Each chain names the class and the method at every hop. `psxport:` marks a framework hop.

### The frame turn

`main` (`game/entry/player_entry.cpp`) → `psxport_install_game(runtime)` → `psxport:native_boot_run(core)`
→ `psxport:FrameLoopShell::prepareProduct` → the loop's `psxport:FrameLoopShell::step(core, frame)` →
**`CrashBashFrameDriver::stepFrame`** (the title's single frame owner) → the guest process state's
update/present pair → the guest's own `DisplayFrame` override →
`CrashBashFrameDriver::deliverDisplayFields` → back at the driver tail, one
`Game::presentation.commit(core, fields, game.temporalPresentation.get())` → `psxport:GpuState` fence
advances once, which `FrameLoopShell::step` asserts.

- `stepFrame` never delegates frame ownership to guest VSync. It brackets the frame for
  `Game::perf`, and its phases are a **partition**, not a nest: the per-field SPU advance stays
  inside the GameLogic span, so the `audio` slot reports 0.00 by construction.
- **While a blocking load or presentation blocks**: it is still `CrashBashFrameDriver::stepFrame`.
  There is no second loop and no second input pump in this title. The blocking work is reached from
  `deliverDisplayFields` → `runtime::dispatchGuest(core, guest::kVblankRoot)` or from a
  `measuredGuestCall`, and each of those is a bounded executor exit the driver resumes itself; a
  guest call needing more than one display field is resumed by
  `crashbash::runtime::runGuestCallToReturn` under the `kGuestCallTurnCap` fence. Host input is not
  re-pumped here because `Game::pad.serviceFrame()` already ran at the top of `stepFrame` and
  psxport's `Pad::pollHostInput` is the single host pump.

### Host input → guest pad buffer

`psxport:psx::input::HostInput` (the SDL/gamepad mask) → `psxport:Pad::pollHostInput` (the one host
pump) → `psxport:Pad::serviceFrame` at the top of `CrashBashFrameDriver::stepFrame`, which resolves
forced / REPL / replay input and records or replays the frame → `psxport:Pad::fillBuffer` writes the
standard digital packet into the guest's VBlank slot.

- **Recording/replay key**: `crashbash::InputPhase::of(core)`, returned by
  `TitleAdapter::inputPhase`. psxport owns segment persistence and phase matching.
- **Movie / skip and any other consumer of button edges**: `psxport:Pad::sampleButtonEdges` /
  `pressedButton`. The guest's own state machine decides what an edge means; nothing here consumes
  or suppresses game input. This title has no movie player, so the logo cancel it does own is
  `crashbash::boot_logo_skip` (Start/Cross through the scene lifecycle, never a timer).
- **Debug control channel**: `psxport:DbgServer::service` at the loop tail →
  `psxport:DbgServerInternals::handleCommand` → `GameRuntime::controlCommand` →
  **`TitleAdapter::controlCommand`** → `crashbash::debug::DevArena::handle`. The command only arms
  a request; `CrashBashFrameDriver::stepFrame` advances it through `DevArena::applyArmed` using the
  title's own pad (`Pad::driveTap`), which is the same path a player's press takes.

### Guest draw → presentation

Guest submit leaf (`0x8001965C` model / `0x800193A8` packet / the 2D quad leaves) →
`registerModelSubmitCaptureOverrides` / `registerSpriteQuadCaptureOverride` decode the call into a
`render::ModelDraw` / `render::SpriteQuadDraw` and `CrashBashFrameDriver::sceneSnapshots().record(...)`
stores it immutably → `registerDisplayFrameOverride` calls `render::submitFixedModels(core,
snapshots.presentable())` and `render::submitSpriteQuads(core, snapshots.current(), orderingTable)` →
the guest's own OT walk submits the frame to psxport's native queue →
`Game::presentation.commit(...)` →
`crashbash::render::InterpolatedScenePresentation::present` (the 60 fps in-between; it rebuilds only
the native model block from the two completed snapshots and never advances simulation) →
`psxport:GpuState` presents.

- **Real field vs in-between**: `psxport:GpuState::s_interpolated_frames` is the second counter; the
  `frame` control-channel line reports `frame=`, `interp=`, `total=` so a 60 Hz claim cannot be read
  off the real-field count alone.
- **Widescreen**: a projection/viewport/scissor change, not a captured-input change.
  `render::native_model_producer` widens the clip columns through `render::widenedViewportColumns`,
  `render::native_sprite_quad_producer` shifts the 2D canvas, and every fixed 4:3 screen-space
  element is corrected through `crashbash::render::ui_anchor::correctionAndReport` with the class
  declared by `crashbash::render::hud_layout::anchorFor`. Guest memory is untouched.
- **A frame that presented nothing** (`deliveredFields_ == 0`) goes to
  `Game::presentation.commitUnpresented` and `SceneSnapshotHistory::markUnpresented`, so a temporal
  pair is never built from a stale snapshot.

### CD / streaming

Guest CD call → `registerCdStartupOverride` / `registerCdLicenseStartupOverride` /
`registerCdFileReadOverride` (all in `game/disc/`) → the descriptor-relative read writes sectors
into guest RAM, and each sector write calls `crashbash::retireImagesForCdSectorWrite` →
`crashbash::runtime::retireAuthenticatedImagesForWrite` → `GuestExecution::retireImagesOverlapping`,
which subtracts the overwritten bytes from the authenticated generation and retires the native keys
at overwritten entries → the completed read is offered to the publication owners
(`isBootImageRead`/`isMenuImageRead`/`NestedModuleImage::offer`) → `completeModuleImageRead`
authenticates guest RAM and publishes a fresh generation →
`GuestExecution::bindAuthenticatedImage` → `crashbash::diagnostics::RunLedger::noteImagePublication`.

- The five nested overlays reuse the `0x800B32B4` slot, so a publication here necessarily retires the
  previous occupant; `NestedModuleImage` exists because without it a nested load would leave its own
  code with no image identity, which the executor refuses as a typed fault.
- The retail load queue's pacing pump (`0x8001231C`) is drained to quiescence by
  `registerLoadPumpDrainOverride`, from the pump's own three queue words. That is the whole of the
  title's loading-removal policy; the LOADING card is retired separately by
  `registerLoadingCardSkipOverride` at its two measured call sites.

### Audio

The guest's own SPU writes in the VBlank root the frame driver dispatches
(`deliverDisplayFields` → `psxport:dispatchGuest(kVblankRoot)`); `Game::spu_audio.frame()` advances the
host SPU once per delivered field. There is no title-owned mixer: XA/XADPCM playback and the Beetle
SPU are psxport's (`psxport:xa_bind`, `spu_bind`, `xa_*`), and `PSXPORT_NOAUDIO` is the silent-run
switch. The frame-time `audio` slot is 0.00 by construction (see the frame turn above), not measured.

### The debug / control channel

`psxport:DbgServer` (loopback TCP, started by `psxport:native_boot_run`; always on unless the port is
taken) → `handleCommand`: framework commands first (`r/w/regs/press/shot/dumpram/step/play/state/
session/frame`), then `GameRuntime::controlCommand` → `TitleAdapter::controlCommand` →
`crashbash::debug::DevArena::handle`. The run-end numbers come from
`crashbash::diagnostics::RunLedger` (fed from `GuestExecution`, the publication owners and
`player_entry.cpp`), and a refusal goes through `RunLedger::refuseRun`, the product's one refusal
route, so a run that dies still reports.

## Placement rules for new work

- A native override belongs in the directory of the thing it replaces, next to its sibling overrides,
  and is added to `game/execution/native_owner_set.cpp` — never registered somewhere else.
- A guest address or a guest-struct layout belongs in `game/title/crashbash_guest.h` as an
  `inline constexpr`; nothing else declares a guest address.
- A pure screen-space policy belongs in `game/render/widescreen/`, a temporal one in
  `game/render/fps60/`, and neither may read or write guest memory.
- A run-lifetime number belongs in `diagnostics::RunLedger` as a fact with a feeder at the site the
  fact happens, not as a tally beside the code that produces it.
- This codemap is updated in the same change whenever responsibility moves or a new owner appears.
