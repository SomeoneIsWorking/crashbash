# Codemap

| Subsystem | Status | Where | Gap / next |
|---|---|---|---|
| Framework consumer | 🟡 booting | `CMakeLists.txt`, `game/core/`, `external/psxport/`, `psxport.pin` | Pinned psxport 3418a79b deterministic drive timing removes the 189-sector retry and reaches loaded entry 0x80092BDC with no DMA depletion; shared CD response/IRQ coalescing still hides the oracle-visible completion-pending return |
| C++ verification | ✅ enforced | `.clang-format`, `.clang-tidy`, `CMakeLists.txt`, `external/psxport/tools/check_cpp_style.py` | Normal CTest checks all 4 first-party TUs for format, 1,200-line ownership, and compile-database clang-tidy; generated/framework code stays outside the first-party gate |
| Project tools | ✅ boot pipeline | `tools/run.py`, `tools/provision.py`, `tools/recomp_bootstrap.py`, `tools/verify_boot.py`, `tools/verify_oracle_irq.py`, `tools/verify_read_completion.py`, `tools/psxport_sync.py`, `tests/test_provision.py` | Default launcher owns sync→Clang build→provision→emit→product and preserves an explicit disc path for runtime; identity, emitter, live-boundary, oracle-order, and read-completion verifiers each demonstrate both answers |
| Title integration | 🟡 resident substrate | `titles/crashbash/executable.json`, `game/recomp_seeds.json`, `generated/` (gitignored) | Real USA image emits 340 roots / 908 resident functions, including the retail-derived HookEntryInt re-entry at 0x80031AE8; no overlays are configured |
| Native engine | 🟡 boot seam only | `game/core/game_config.cpp`, `game/core/game_hooks.cpp`, `game/core/recomp_register.cpp`, `game/core/main.cpp` | PC side owns measured configuration, routing, and composition only; no game behavior has been replaced |
| Native graphics producers | ⬜ missing | — | The native graphics producer directory does not exist |
| Widescreen | ⬜ missing | — | Blocked on native camera and producers |
| Interpolation | ⬜ missing | — | Blocked on PC ownership of transform producers |
| Differential harness | 🟡 read-completion boundary | `tools/verify_oracle_irq.py`, `tools/verify_read_completion.py`, `tools/verify_boot.py`, `game/core/` | Independent Beetle proves returned 189 then completion 1→0. The deterministic port now matches start and live progression but exposes completion 1 only transiently inside IRQ handling, so the strict comparator rejects it. Full-memory lockstep remains unavailable |
