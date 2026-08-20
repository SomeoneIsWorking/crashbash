# Crash Bash — selected target

The selected target is the North American retail disc (`SCUS-94570`, NTSC-U). This is an identity and
boot-image measurement, not a boot claim: the repo still has no game seam, generated substrate, or
native executable.

`SYSTEM.CNF` names `cdrom:\SCUS_945.70;1`. The executable independently contains both the
`Sony Computer Entertainment Inc. for North America area` and `BASCUS-94570` markers. The disc also
contains a Spyro 3 demo payload, but that is not the configured boot executable.

| executable fact | measured value |
|---|---|
| disc path | `SCUS_945.70` |
| file size | 432,128 bytes |
| SHA-1 | `c4a06208612ff2625b9083596d36fdf60b01be5f` |
| SHA-256 | `fd5727a18feb2a2d5a6359a55966f0266284d1e50f64ee9b8a127a97091bd516` |
| entry PC | `0x8002E7B0` |
| text mapping | `0x80010000..0x80079000` (`0x69000` bytes, all present) |
| header GP | `0x00000000` |
| header SP | `0x801FFFF0` |

The tracked machine-readable identity is `executable.json`.

## CRT0 evidence

psxport's shipping CRT0 decoder scanned 36 instructions from the real entry, reached the InitHeap call,
and resolved 8/8 boot fields:

| field | measured value |
|---|---|
| BSS zero range | `0x8006E9F0..0x80078C90` (41,632 bytes) |
| stack-top word | `0x8002E860` (`0x00200000`) |
| second stack word | `0x8006D8B4` (`0x00008000`) |
| runtime SP/FP | `0x80200000` |
| runtime GP | `0x8006E9EC` |
| heap | `0x80078C90`, `0x17F370` bytes; InitHeap receives `0x80078C94` |
| InitHeap thunk | `0x8003ACCC` |
| heap globals | absent; values remain register-only in this CRT0 |

These values are evidence for the future boot seam. They are not yet shipped in a `GameConfig`, so the
runtime CRT0 audit has not compared them and this project is not claimed to boot.

## Reproduce the measurement

Build the framework tools explicitly with Clang, then inspect and extract from a provisioned disc:

```sh
CCACHE_DISABLE=1 cmake -S . -B scratch/build-clang \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
CCACHE_DISABLE=1 cmake --build scratch/build-clang --target discdump crt0_extract -j16
scratch/build-clang/psxport_build/tools/discdump list "$CRASHBASH_DISC"
scratch/build-clang/psxport_build/tools/discdump get SYSTEM.CNF "$CRASHBASH_DISC" scratch/raw/crashbash-usa
scratch/build-clang/psxport_build/tools/discdump get SCUS_945.70 "$CRASHBASH_DISC" scratch/raw/crashbash-usa
scratch/build-clang/psxport_build/tools/crt0_extract scratch/raw/crashbash-usa/SCUS_945.70
```

Disc images and extracted files stay under external storage or gitignored `scratch/`; neither is tracked.
For normal provisioning, `python3 tools/provision.py [disc.chd]` owns disc resolution, validates
`SYSTEM.CNF`, checks these same 11 executable facts, and atomically publishes only a verified image.
