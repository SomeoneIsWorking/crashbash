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
| Generated function registration and overlay routing | `game/core/recomp_register.cpp`, generated dispatch/overlay tables | Title registration stays here; generic routing changes belong in psxport, not a game-local copy |
| Boot and oracle boundary verification | `tools/verify_boot.py`, `tools/verify_oracle_irq.py`, `tools/verify_read_completion.py`, `tools/verify_command_response_timing.py`, `tools/verify_cdc_phase_progress.py` | Each new boundary gets one title-local judge that exercises shipping output and controlled opposite answers |
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
