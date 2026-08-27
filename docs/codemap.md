# Codemap

This map records responsibility and placement only. Capability coverage and blockers live in
`docs/project-state.md`; evidence order lives in `docs/re-frontier.md`.

| Responsibility | Current owner / location | New responsibility goes |
|---|---|---|
| Player launch and portable provisioning orchestration | `run.sh`, `bootstrap.py`, `pyproject.toml`, `uv.lock` | Platform discovery and build policy stay in `bootstrap.py`; `run.sh` remains a slim shim |
| Framework consumption and C++ product build | `CMakeLists.txt`, `external/`, specifically external/psxport | Game-specific composition stays local; reusable runtime behavior belongs in the shared framework checkout |
| Retail disc, executable, and loaded-module identity | `tools/provision.py`, `tools/loaded_module.py`, `titles/`, specifically titles/crashbash | New restricted-input measurements join the title manifests and the authoritative provision path |
| Generated-code derivation and cache integrity | `tools/recomp_bootstrap.py`, `game/recomp_seeds.json`, `generated/` (ignored) | New seeds require observed control-flow evidence in `game/recomp_seeds.json`; generated output is never edited |
| Host composition and title runtime | `game/core/main.cpp`, `game/core/crashbash_runtime.cpp`, `game/core/legacy_game_interface.h` | Composition stays in `main.cpp`; cohesive title behavior gets a dedicated module outside the entry point |
| Typed resident ownership facts | `game/core/crashbash_guest.h` | Guest entry points, process-state addresses, and exact platform-service windows join this typed title-local declaration and must be checked against retail bytes |
| Finite boot and lifetime process-runner activation | `game/core/crashbash_boot.{h,cpp}` | Port more of the one-shot retail `0x8002718C` / `0x80010158` prefix here; repeating work belongs to the FrameDriver, never a guest main loop |
| Native frame/process/display ownership | `game/core/crashbash_frame_driver.{h,cpp}`, `game/core/display_frame.{h,cpp}` | Process-state iteration stays finite in the driver; RE-derived display/arena work stays in its cohesive owner, calls no guest VSync, and contributes to the driver's single presentation commit |
| Synchronous GPU timeout and transfer ownership | `game/core/gpu_timeout.{h,cpp}` | GPU timeout/transfer roots retain generated supers here; the native path is finite because host submission is synchronous and never samples guest VSync |
| Synchronous CD startup ownership | `game/core/cd_startup.{h,cpp}`, measured libcd bindings in `game/core/game_config.cpp` | The title-local controller handshake retains its generated super here; reusable command, sync, and ISO lookup behavior stays in psxport's native CD subsystem |
| Synchronous CD file reads | `game/core/cd_file_read.{h,cpp}` | Descriptor-relative Crash Bash file loads retain their generated async super here; the native owner returns success only after every real CHD sector is copied into guest RAM |
| Synchronous disc/license startup | `game/core/cd_license_startup.{h,cpp}` | The 20-state retail controller sequence retains its generated super here; the native owner binds the runtime medium to the measured SCUS-94570 layout and enters the retail passed/idle state without a guest delay clock |
| Memory-card BIOS/vector startup ownership | `game/core/memory_card_startup.{h,cpp}` | Boot-time card initialization retains its generated super here; the native owner preserves the measured critical-section and BIOS/vector setup without a guest frame wait |
| Generated function registration and overlay routing | `game/core/recomp_register.cpp`, generated dispatch/overlay tables | Title registration stays here; generic routing changes belong in psxport, not a game-local copy |
| Boot, native-ownership, and oracle boundary verification | `tools/verify_boot.py`, `tools/verify_native_ownership.py`, `tools/verify_oracle_irq.py`, `tools/verify_read_completion.py`, `tools/verify_command_response_timing.py`, `tools/verify_cdc_phase_progress.py` | Each new boundary gets one title-local judge that exercises shipping output and controlled opposite answers |
| Shipping nested-MENU entry diagnostic | `game/diagnostics/`, specifically `game/diagnostics/menu_boundary.{h,cpp}` | Behavior-preserving entry observers stay title-local, log once, and super-call the retained generated body; reusable diagnostic machinery belongs in psxport |
| Hermetic provisioning and launcher tests | `tests/` | New tool/launcher behavior gets positive and refusal coverage here through the shipping implementation |
| Retail render-anchor discovery | `tools/inventory_render_anchors.py`, `docs/findings/render-anchor-inventory.md` | Static camera/submitter candidates and call ancestry stay in this analyzer/finding; semantic decompilation evidence joins the finding |
| First-frame projection attribution | `tools/verify_render_anchor_reach.py`, framework `rtpcaller` diagnostic | Title-local runtime ancestry judgment stays in this verifier; reusable GTE caller observation stays in psxport |
| Native game-state scene snapshots | unassigned; placement root `game` | Decoded, immutable per-simulation-tick camera/object/material state goes to render/scene_snapshot.{h,cpp} once RE proves its retail inputs |
| Native camera and projection | unassigned; placement root `game` | 4:3-equivalent view/projection and explicit aspect policy go to render/camera_projection.{h,cpp}; never patch guest GTE or GP0 coordinates |
| Native graphics producers | unassigned; placement root `game` | Create cohesive modules under render/producers/, one per proven scene responsibility, consuming decoded snapshots through psxport's native render seam |
| Presentation interpolation | unassigned; placement root `game` | Interpolation goes to render/interpolated_scene.{h,cpp}, consumes two immutable native snapshots and render alpha, and never mutates simulation or guest RAM |
| Project facts and atomic work | `docs/project-state.md`, `docs/re-frontier.md`, `docs/issues/`, `docs/info/` | Capability facts go to project state, RE dependency evidence to the frontier, atomic work to issues, claims/instruments to info |

The future game/render ownership follows Dusklight's separation of frame interpolation from camera
operators and host composition. Files are targets, not empty scaffolding; create each only when its
first RE-grounded responsibility is implemented.
