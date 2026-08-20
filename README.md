# Crash Bash

PC-native PlayStation port of Crash Bash, built on
[psxport](https://github.com/SomeoneIsWorking/psxport).

Current status: the North American target and its executable/CRT0 metadata are measured, and its boot
executable is reproducibly provisioned and verified from external media. No game seam, generated
substrate, runnable game executable, oracle comparison, native producer, widescreen path, or
interpolation path is claimed yet.

## Configure the framework scaffold

```sh
python3 tools/psxport_sync.py --auto
CCACHE_DISABLE=1 cmake -S . -B scratch/build-clang \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
CCACHE_DISABLE=1 cmake --build scratch/build-clang --target crashbash_scaffold -j16
```

Run `ctest --test-dir scratch/build-clang --output-on-failure -R crashbash_cpp_policy` for the normal
first-party C++ gate. The shared framework checker applies this repository's tracked `clang-format`
and `clang-tidy` policy and the 1,200-line ownership cap without linting `external/psxport` or
generated code. The scaffold currently reports an explicit zero first-party translation units.

## Provision the selected retail executable

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

Disc images and extracted executables are never committed. The measured identity remains in
`titles/crashbash/executable.json`; successful provisioning does not claim recompilation or boot.
