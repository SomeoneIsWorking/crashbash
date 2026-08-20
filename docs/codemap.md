# Codemap

| Subsystem | Status | Where | Gap / next |
|---|---|---|---|
| Framework consumer | 🟡 scaffold | `CMakeLists.txt`, `external/psxport/`, `psxport.pin` | Clang-built smoke target verified against psxport `be381503`; no game seam |
| C++ verification | ✅ enforced | `.clang-format`, `.clang-tidy`, `CMakeLists.txt`, `external/psxport/tools/check_cpp_style.py` | Normal CTest runs shared first-party-only format, 1,200-line ownership, and compile-database clang-tidy checks; currently 0 translation units |
| Project tools | ✅ provisioning | `tools/psxport_sync.py`, `tools/provision.py`, `tests/test_provision.py` | Framework synchronization plus CLI/env/.env/drop-in disc resolution, `SYSTEM.CNF` and 11-fact executable verification, and atomic gitignored publication are covered by both-answer tests |
| Title integration | 🟡 provisioned | `titles/crashbash/executable.json`, `tools/provision.py` | Real USA `SCUS_945.70` provisions at the tracked SHA-256; derive recompilation seeds next, without tracking generated output |
| Native engine | ⬜ missing | — | No `game/` directory or owned game code |
| Native graphics producers | ⬜ missing | — | The native graphics producer directory does not exist |
| Widescreen | ⬜ missing | — | Blocked on native camera and producers |
| Interpolation | ⬜ missing | — | Blocked on PC ownership of transform producers |
| Differential harness | ⬜ missing | — | Blocked on recompilation seeds and generated substrate; stand up oracle before game logic |
