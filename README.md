# Crash Bash

Crash Bash is being migrated to one native/Lightrec hybrid over the authenticated USA PlayStation
release. The title retains its cohesive native boot, device, rendering, input, widescreen, and
interpolation owners. Every other guest path will execute through psxport's per-`Core` Lightrec
runtime.

The old offline translator integration, generated guest corpus, static dispatcher/registry, seed
configuration, static-only verifiers, build tree, and static product binaries were deleted before
dynarec implementation. They are not a bridge, fallback, or oracle.

The gameplay target is intentionally unavailable while psxport's Lightrec backend and loaded-image
lifecycle binding are incomplete. `CMakeLists.txt` refuses `crashbash_port` with that exact boundary.
The surviving native code now has one declared adapter in `game/core/guest_execution.h`:

- 27 native override registrations are keyed by logical authenticated image and guest address.
- 15 former generated-body calls use a scoped `callOriginal` operation.
- ordinary native-to-guest calls use one runtime dispatch operation.

The adapter will bind these operations to psxport's generation-aware Lightrec override registry,
original-call suppression, and code-cache invalidation. It must not contain or select an interpreter.

## Player input and provisioning

The intended default remains `./run.sh`, with an optional explicit USA CHD:

```sh
./run.sh
./run.sh "/path/to/Crash Bash (USA).chd"
```

The launcher currently provisions the authenticated executable and modules, then stops at the
explicit missing Lightrec adapter target. It never emits or compiles guest source. Disc resolution is
explicit argument, `PSXPORT_CRASHBASH_DISC`, `PSXPORT_DISC`, the same keys in `.env`, then one
root-level `.chd`. Original game content remains untracked.

`run.sh` is only a stable root handoff to `uv run --frozen python bootstrap.py`. Python owns
dependency discovery, provisioning, build policy, and launch behavior; it passes the locked
interpreter through to project tools.

## Verification during the broken-first phase

These checks do not build or run the removed product:

```sh
uv run --frozen python tools/verify_native_ownership.py
uv run --frozen python tests/test_source_policy.py
uv run --frozen python tests/test_provision.py
```

The source-policy check reports the complete 27-registration and 15-original-call denominators and
rejects generated directories, old dispatch surfaces, or retired tools. Historical emulator/binary
evidence and the bounded replays under `replays/flow/` remain the acceptance scenarios for later
dynarec requalification.

## Product completion

After the shared executor lands, the port must prove nonzero translated execution, correct
loaded-image invalidation, bounded VSync/interrupt/exception exits, image-scoped overrides, and an
interpreter-free gameplay link. Boot, logos, menus, FMV, and a first frame are implementation
checkpoints only. Completion requires representative controllable gameplay with correct rendering,
audio, timing, devices, and frame-time evidence, followed by broader retail-mode coverage.

See `docs/project-goals.md`, `docs/project-state.md`, `docs/codemap.md`, and
`docs/re-frontier.md` for the canonical goal, state, ownership, and evidence boundaries.
