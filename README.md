# Crash Bash

PC-native PlayStation port of Crash Bash, built on
[psxport](https://github.com/SomeoneIsWorking/psxport).

Current status: the North American executable is reproducibly provisioned, its resident substrate is
emitted and verified, and the Clang-built port's custom interrupt exit agrees with an independent
Beetle interpreter running the real disc. The port reaches the master dispatcher and CD IRQ callback
without a recomp miss or the former CD timeout. Real CDC/DMA traces now provision and emit the initial
BOOT and nested MENU code modules from `CRASHBSH.DAT`; the port executes both and prints `empty prims`.
Its next honest boundary is shared CD command timing: psxport makes GetTN's response available on the
command write, so the interrupt handler drains and acknowledges it before Crash Bash enters resident
poll state `0x8002DE2C`. No full-memory lockstep, game frame, native graphics producer, widescreen
path, or interpolation path is claimed yet.

## Run the current product

`./run.sh` is the default product path. With no arguments it resolves media through the normal
environment / `.env` / drop-in policy; an optional USA CHD path may be supplied explicitly. The thin
launcher delegates sync, Clang configuration, provisioning, emission, and the product build to
`tools/run.py`.

## Build and verify explicitly

```sh
python3 tools/psxport_sync.py --auto
CCACHE_DISABLE=1 cmake -S . -B scratch/build-clang \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
CCACHE_DISABLE=1 cmake --build scratch/build-clang --target discdump crt0_extract -j16
python3 tools/provision.py "/path/to/Crash Bash (USA).chd"
python3 tools/recomp_bootstrap.py --ensure \
  --crt0-extract scratch/build-clang/psxport_build/tools/crt0_extract
CCACHE_DISABLE=1 cmake -S . -B scratch/build-clang \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
CCACHE_DISABLE=1 cmake --build scratch/build-clang --target crashbash_port -j16
PSXPORT_CRASHBASH_DISC="/path/to/Crash Bash (USA).chd" \
  ctest --test-dir scratch/build-clang --output-on-failure
```

The normal CTest suite includes real and forced-negative recompilation/boot-boundary checks, a
forced-negative oracle/port interrupt-order comparator, a command-response-order diagnostic, and the
first-party C++ gate. The shared checker applies this repository's tracked `clang-format` and
`clang-tidy` policy and the 1,200-line ownership cap to all five first-party translation units without
linting `external/psxport` or generated code.

## Provision the selected retail inputs

Build `discdump` with Clang, then run the title-local provisioner:

```sh
CCACHE_DISABLE=1 cmake --build scratch/build-clang --target discdump -j16
python3 tools/provision.py "/path/to/Crash Bash (USA).chd"
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
