# Codemap

| Subsystem | Status | Where | Gap / next |
|---|---|---|---|
| Framework consumer | 🟡 booting | `CMakeLists.txt`, `game/core/`, `external/psxport/`, `psxport.pin` | Four-TU seam and generated adapter are verified against psxport `2b5ef7b5`; CD/VSync hardware service is the next boundary |
| C++ verification | ✅ enforced | `.clang-format`, `.clang-tidy`, `CMakeLists.txt`, `external/psxport/tools/check_cpp_style.py` | Normal CTest checks all 4 first-party TUs for format, 1,200-line ownership, and compile-database clang-tidy; generated/framework code stays outside the first-party gate |
| Project tools | ✅ boot pipeline | `tools/run.py`, `tools/provision.py`, `tools/recomp_bootstrap.py`, `tools/verify_boot.py`, `tools/psxport_sync.py`, `tests/test_provision.py` | Default launcher owns sync→Clang build→provision→emit→product; identity, emitter, and live-boundary verifiers each demonstrate both answers |
| Title integration | 🟡 resident substrate | `titles/crashbash/executable.json`, `game/recomp_seeds.json`, `generated/` (gitignored) | Real USA image emits 339 roots / 907 resident functions and reaches guest main plus the measured IRQ callback; no overlays are configured |
| Native engine | 🟡 boot seam only | `game/core/game_config.cpp`, `game/core/game_hooks.cpp`, `game/core/recomp_register.cpp`, `game/core/main.cpp` | PC side owns measured configuration, routing, and composition only; no game behavior has been replaced |
| Native graphics producers | ⬜ missing | — | The native graphics producer directory does not exist |
| Widescreen | ⬜ missing | — | Blocked on native camera and producers |
| Interpolation | ⬜ missing | — | Blocked on PC ownership of transform producers |
| Differential harness | 🟡 port boundary | `tools/verify_boot.py`, `game/core/` | Port-side verifier proves CRT0 audit, guest-main/IRQ reachability, no recomp miss, and the CD/VSync stop; a deterministic true-oracle comparison is still missing |
