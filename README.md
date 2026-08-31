# Crash Bash

PC-native PlayStation port of Crash Bash, built on
[psxport](https://github.com/SomeoneIsWorking/psxport).

The port source is distributed under GPL-3.0-or-later; see `COPYING`. Original Crash Bash game
content is not included and must be supplied by the player.

Current status: the North American executable plus BOOT and five measured nested modules are
reproducibly provisioned and emitted into a 2,698-function substrate. The tracked
`replays/flow/crashball-control.pad` reaches and controls a live Crashball match; the tracked
`replays/flow/battle-crate-crush-control.pad` reaches and controls a live Crate Crush match through
its title-owned objective, controls, and special-items pages. Both shipping-native paths complete
without a guest-VSync violation, timeout, watchdog, recompilation miss, or fatal. Native producers
render the arenas, players, HUD, briefing layers, crates, balls, and items from title-owned state.
Gameplay uses a centred wide camera; an authored briefing remains a complete centred 4:3 composition.
`PSXPORT_FPS60=1` presents interpolated midpoints from immutable scene snapshots. Broader game-mode
coverage and full native parity remain in progress.

## Run the current product

`./run.sh` is the fresh-clone and default product path. With no arguments it resolves media through
the normal environment / `.env` / drop-in policy, builds `crashbash_port`, and opens the current
player. An optional USA CHD path may be supplied explicitly:

```sh
./run.sh
./run.sh "/path/to/Crash Bash (USA).chd"
```

The only Python prerequisite is `uv`. The launcher enters the checked-in `uv.lock`, passes that exact
Python interpreter to CMake and every project tool, and does not require Ghidra. It honors `CC` and
`CXX` when set; otherwise CMake selects the available C and C++ compilers. The player path does not
whitelist or blacklist compiler identities.

Native prerequisites can be checked without syncing, provisioning, building, or launching:

```sh
./run.sh --check
```

Missing dependencies are refused with an exact `sudo dnf install ...`, `sudo apt install ...`, or
`brew install ...` command selected for the host. Run `./run.sh --prepare-only [disc.chd]` to complete
the same provisioning and product build without opening a window. Neither non-launching mode runs
CTest.

`run.sh` is deliberately only a stable repository-root handoff to
`uv run --frozen python bootstrap.py`; dependency discovery, sync, provisioning, build policy, and
launch behavior have one owner in `bootstrap.py`. The player uses the isolated
`build/player` tree with `BUILD_TESTING=OFF` and builds only `crashbash_port`. Compiler output,
package caches, and release artifacts share the top-level `build/` owner; `scratch/` is reserved for
disposable runtime captures, logs, and extracted restricted inputs.

## Build and verify explicitly

```sh
LOCKED_PYTHON="$(uv run --frozen python -c 'import sys; print(sys.executable)')"
uv run --frozen python tools/psxport_sync.py --auto
CC=clang CXX=clang++ cmake --fresh -S . -B build/maintainer \
  -DPython3_EXECUTABLE="$LOCKED_PYTHON" -DPython3_FIND_VIRTUALENV=ONLY
cmake --build build/maintainer --target discdump crt0_extract -j16
uv run --frozen python tools/provision.py "/path/to/Crash Bash (USA).chd"
uv run --frozen python tools/recomp_bootstrap.py --ensure \
  --crt0-extract build/maintainer/psxport_build/tools/crt0_extract
CC=clang CXX=clang++ cmake -S . -B build/maintainer \
  -DPython3_EXECUTABLE="$LOCKED_PYTHON" -DPython3_FIND_VIRTUALENV=ONLY
cmake --build build/maintainer --target crashbash_port -j16
PSXPORT_CRASHBASH_DISC="/path/to/Crash Bash (USA).chd" \
  ctest --test-dir build/maintainer --output-on-failure
uv run --frozen python -m unittest discover -s tests -p 'test_*.py'
```

The normal CTest suite includes real and forced-negative recompilation/boot-boundary checks, a
forced-negative oracle/port interrupt-order comparator, a command-response-order diagnostic, and the
positive CDC phase-progress verifier selftest. The shared checker applies this repository's tracked `clang-format` and
`clang-tidy` policy and the 1,200-line ownership cap to all five first-party translation units without
linting `external/psxport` or generated code.

The positive-progress verifier can audit a recorded trace without launching, or run the already-built
headless product directly. Its runtime path terminates only its exact child PID after reaching LBA
17655; a timeout is a refusal, never success:

```sh
uv run --frozen python tools/verify_cdc_phase_progress.py --selftest
uv run --frozen python tools/verify_cdc_phase_progress.py --trace scratch/logs/crashbash-cdc-phases-once.log
uv run --frozen python tools/verify_cdc_phase_progress.py
```

## Provision the selected retail inputs

Build `discdump` with Clang, then run the title-local provisioner:

```sh
cmake --build build/maintainer --target discdump -j16
uv run --frozen python tools/provision.py "/path/to/Crash Bash (USA).chd"
```

With no argument, disc resolution is `PSXPORT_CRASHBASH_DISC`, then `PSXPORT_DISC`, the same keys in
`.env`, then one `*.chd` drop-in at the repository root. A configured missing path or multiple drop-ins
refuse instead of silently choosing different media. The tool extracts into a scoped temporary
directory, checks the `SYSTEM.CNF` boot target and all 11 tracked executable identity/header facts, and
only then atomically publishes `scratch/bin/crashbash/SCUS_945.70`.

The same transaction extracts `CRASHBSH/CRASHBSH.DAT` once, verifies its complete 73,220,096-byte
identity, then verifies and publishes BOOT and MENU payloads under
`scratch/bin/crashbash/overlays/`. Their tracked manifests bind disc LBA, DAT offset, payload hash,
RAM range, and observed entry pointer; a mismatch publishes none of the executable or module inputs.

Disc images, extracted executables, and `generated/` are never committed. The measured identity
remains in `titles/crashbash/executable.json`; module identities remain in
`titles/crashbash/boot_module.json` and `titles/crashbash/menu_module.json`.
`tools/recomp_bootstrap.py` independently rechecks all three inputs, their emitted interfaces/ranges,
and every shipping CRT0 legacy program-fact binding.
