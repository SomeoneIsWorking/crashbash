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

Disc images and extracted executables are never committed. Provisioning resolution is the next frontier
step; the measured target identity is in `titles/crashbash/executable.json`.
