# Crash Bash port

Read `external/psxport/CLAUDE.md` and `external/psxport/docs/workspace/PROTOCOL.md` before work.
Generated code is sacrosanct. Never commit discs, extracted executables, `generated/`, `.env`, or
machine-specific paths. Run artifacts go under `scratch/`, never `/tmp`.

**`external/psxport` is NOT a git submodule** (2026-08-16): it is a symlink to the workspace's shared
framework clone when one exists, or a private clone at this repo's `psxport.pin` on a fresh machine.
`tools/psxport_sync.py --auto` establishes whichever applies; `psxport_sync.py --bump` records the
framework commit this game is built and VERIFIED against, and `--check` fails when the built framework
is not the recorded pin. Framework edits happen in the shared clone (`$PSX/psxport`), never here.

All picture work is RE-driven. Widescreen and interpolation require PC-native graphics producers
reading game state; do not reconstruct pictures from GTE/OT/GP0 output.

USER 2026-08-30: "Change the directive, pixel matching doesn't matter. I just want working game that
looks correct."

The bar is a working game that looks correct, not pixel-exact agreement with the PSX reference. This is
a WORKSPACE-WIDE rule now — `external/psxport/CLAUDE.md` bar 3 is the authority and
`external/psxport/docs/workspace/PROTOCOL.md` carries it with the incident it came from. Frame
comparison, the retail oracle, and the differential harness are DIAGNOSTICS for finding the cause of a
visible defect; they are never the completion condition, and nothing is held open by a residual pixel
count. Faithfulness of execution is unchanged: simulation, input, timing, audio, and scene semantics
still come from the real executable and real assets.

Drive the game and look at it before believing a picture claim:

    python3 external/psxport/tools/port/looks_right.py \
        --binary build/bin/crashbash_port --frames 3740 --shot-at 3700 \
        --replay replays/flow/crashball-control.pad

Measured 2026-08-30: reaches PASS, widescreen PASS, fps60 FAIL (3,739 duplicate presents, issue 0025).

The host structure follows Dusklight's current composition/ownership split: `game/core/main.cpp`
only composes process startup, `CrashBashRuntime` owns framework-facing title behavior through
inheritance, `game_config.cpp` and `game_hooks.cpp` are bounded compatibility facts for generic
framework code that still reads `Core::cfg/Core::hooks`, and `recomp_register.cpp` is the sole
generated adapter. Project automation remains in Python under `tools/`; do not grow the process entry
point or move game behavior into compatibility tables or the generated adapter.

The player entry point is `./run.sh`, a slim repository-root handoff to the frozen uv environment and
root `bootstrap.py`. Zero arguments provision, build, and launch `crashbash_port`; `--check` and
`--prepare-only` are the non-launching paths. The bootstrap passes its exact interpreter into CMake,
honors `CC`/`CXX` without compiler identity policy, and owns platform-specific dependency refusals.
Its isolated `build/player` tree has `BUILD_TESTING=OFF`; tests belong only to the separate
maintainer build.
