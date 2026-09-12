# Crash Bash port

Read `external/psxport/CLAUDE.md` and `external/psxport/docs/workspace/PROTOCOL.md` before work. The local
goals, state, ownership map, and RE frontier are `docs/project-goals.md`, `docs/project-state.md`,
`docs/codemap.md`, and `docs/re-frontier.md`.

Never commit discs, extracted executables, `.env`, or machine-specific paths. Run artifacts go under
`scratch/`, never `/tmp`; build output goes under `build/`.

`external/psxport` is not a git submodule. It is a symlink to the workspace's shared framework clone
when one exists, or a private clone at this repository's `psxport.pin` on a fresh machine.
`tools/psxport_sync.py --auto` establishes that checkout. Framework changes happen in the shared
psxport clone, while this title records only a psxport revision that has passed its own product gates.

## Product execution contract

The gameplay product is a native/dynarec hybrid: Crash Bash installs its title-owned native overrides,
and psxport executes every remaining guest path through its maintained, pinned Lightrec integration.
Every cold block is offered to Lightrec first. A bounded interpreter fallback is allowed only after
the shipping JIT reports an explicit compile/fetch failure, with PC, reason, and execution counts.
Interpreter-only execution remains a separately selected test/diagnostic path.

Do not regenerate, build, or run the static product. Do not add a replacement offline translator,
generated guest corpus, static dispatch table, or precompiled title substrate. The complete static
path has already been deleted before dynarec implementation and must remain absent without a
compatibility mode or tombstone.

Preserve all 27 current native override installations and all 15 original calls from native
owners through psxport's scoped runtime original-call operation, which bypasses
only the current override and executes the authenticated original body through Lightrec. Override and
translated-block identity must include the loaded image generation because several modules reuse the
same guest address range.

## Presentation and structure

All picture work is RE-driven. Native rendering reads decoded game-owned camera, object, material, and
animation state; it never reconstructs the product picture from GTE output, ordering tables, GP0, VRAM,
or framebuffer pixels. Widescreen is a projection/viewport/scissor change, and interpolation uses
consecutive immutable native scene snapshots without mutating guest state.

The completion bar is representative gameplay that works and looks correct. Frame comparison, an
independent emulator, and the separately built test target, including diagnostics, may locate a divergence;
they do not define presentation completion. Boot, logos, menus, attract loops, and one frame are not
representative gameplay and cannot establish product completion.

The host structure is project-owned and split by cohesive responsibility. The old host composition
was removed with its legacy runtime adapter. `game/core/title_adapter.{h,cpp}` composes the direct typed
psxport boundary; boot, frame, device, diagnostics, and render responsibilities remain in dedicated
modules with narrow interfaces. Do not grow the future entry point or runtime adapter into a monolith.

The player entry point remains `./run.sh`, a slim repository-root handoff to the frozen uv environment
and `bootstrap.py`. Its eventual zero-argument path must authenticate the user's game image, build, and
launch the native/dynarec product without offline translation. Tests, diagnostics, and migration gates
use separate explicit commands and never hide behind `run.sh`.
