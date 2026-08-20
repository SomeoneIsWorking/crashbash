# Crash Bash

PC-native PlayStation port of Crash Bash, built on
[psxport](https://github.com/SomeoneIsWorking/psxport).

Current status: the North American target and its executable/CRT0 metadata are measured. No game seam,
generated substrate, runnable game executable, native producer, widescreen path, or interpolation path
is claimed yet.

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

Disc images and extracted executables are never committed. Provisioning resolution is the next frontier
step; the measured target identity is in `titles/crashbash/executable.json`.
