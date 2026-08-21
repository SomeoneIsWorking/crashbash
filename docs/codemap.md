# Codemap

| Subsystem | Status | Where | Gap / next |
|---|---|---|---|
| Framework consumer | 🟡 booting | `CMakeLists.txt`, `game/core/`, `external/psxport/`, `psxport.pin` | Custom exception exit, split PVD DMA, and continuous `ReadN` work through the shared framework; the first 189-sector file read repeats instead of completing |
| C++ verification | ✅ enforced | `.clang-format`, `.clang-tidy`, `CMakeLists.txt`, `external/psxport/tools/check_cpp_style.py` | Normal CTest checks all 4 first-party TUs for format, 1,200-line ownership, and compile-database clang-tidy; generated/framework code stays outside the first-party gate |
| Project tools | ✅ boot pipeline | `tools/run.py`, `tools/provision.py`, `tools/recomp_bootstrap.py`, `tools/verify_boot.py`, `tools/verify_oracle_irq.py`, `tools/psxport_sync.py`, `tests/test_provision.py` | Default launcher owns sync→Clang build→provision→emit→product and preserves an explicit disc path for runtime; identity, emitter, live-boundary, and oracle-order verifiers each demonstrate both answers |
| Title integration | 🟡 resident substrate | `titles/crashbash/executable.json`, `game/recomp_seeds.json`, `generated/` (gitignored) | Real USA image emits 340 roots / 908 resident functions, including the retail-derived HookEntryInt re-entry at 0x80031AE8; no overlays are configured |
| Native engine | 🟡 boot seam only | `game/core/game_config.cpp`, `game/core/game_hooks.cpp`, `game/core/recomp_register.cpp`, `game/core/main.cpp` | PC side owns measured configuration, routing, and composition only; no game behavior has been replaced |
| Native graphics producers | ⬜ missing | — | The native graphics producer directory does not exist |
| Widescreen | ⬜ missing | — | Blocked on native camera and producers |
| Interpolation | ⬜ missing | — | Blocked on PC ownership of transform producers |
| Differential harness | ✅ interrupt boundary | `tools/verify_oracle_irq.py`, `tools/verify_boot.py`, `game/core/` | Independent Beetle/real-disc and port traces agree on saved-context re-entry → master dispatcher; real consumer additionally proves PVD `CD001` and a 189-sector continuous read. Full-memory lockstep remains unavailable |
