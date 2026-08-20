# Codemap

| Subsystem | Status | Where | Gap / next |
|---|---|---|---|
| Framework consumer | 🟡 scaffold | `CMakeLists.txt`, `external/psxport/`, `psxport.pin` | Clang-built smoke target verified against psxport `be381503`; no game seam |
| C++ verification | ✅ enforced | `.clang-format`, `.clang-tidy`, `CMakeLists.txt`, `external/psxport/tools/check_cpp_style.py` | Normal CTest runs shared first-party-only format, 1,200-line ownership, and compile-database clang-tidy checks; currently 0 translation units |
| Project tools | 🟡 partial | `tools/psxport_sync.py` | Framework synchronization exists; disc provisioning and a title-local executable verifier remain missing |
| Title integration | ✅ measured | `titles/crashbash/executable.json` | USA `SCUS_945.70` identity and CRT0 measured; provision it without tracking the disc |
| Native engine | ⬜ missing | — | No `game/` directory or owned game code |
| Native graphics producers | ⬜ missing | — | The native graphics producer directory does not exist |
| Widescreen | ⬜ missing | — | Blocked on native camera and producers |
| Interpolation | ⬜ missing | — | Blocked on PC ownership of transform producers |
| Differential harness | ⬜ missing | — | Blocked on provisioning and generated substrate; stand up oracle before game logic |
