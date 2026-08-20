# Codemap

| Subsystem | Status | Where | Gap / next |
|---|---|---|---|
| Framework consumer | 🟡 scaffold | `CMakeLists.txt`, `external/psxport/` | Clang-built smoke target only; no game seam |
| Title integration | ✅ measured | `titles/crashbash/executable.json` | USA `SCUS_945.70` identity and CRT0 measured; provision it without tracking the disc |
| Native engine | ⬜ missing | — | No `game/` directory or owned game code |
| Native graphics producers | ⬜ missing | — | The native graphics producer directory does not exist |
| Widescreen | ⬜ missing | — | Blocked on native camera and producers |
| Interpolation | ⬜ missing | — | Blocked on PC ownership of transform producers |
| Differential harness | ⬜ missing | — | Blocked on provisioning and generated substrate; stand up oracle before game logic |
