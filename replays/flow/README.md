# `replays/flow/` — representative gameplay scenarios

One active-low PSX pad mask per host frame, little-endian `uint16`, `0xFFFF` = every button released.
They are the minimum retained scenarios for requalifying a title after the static execution path was
retired, not claims that every retail mode is covered. Feed one with
`PSXPORT_PAD_REPLAY=<path>` (the run logs `replay COMPLETE` when the last frame is delivered;
`PSXPORT_NATIVE_FRAMES` is no longer a knob and the runtime warns that it did nothing); the guest receives the masks through the
real SIO0 pad chain, so every transition below is the guest's own.

A file is either unkeyed (masks at absolute frames from boot) or **phase-keyed v1**: a chain of
segments, each keyed by the title's input phase — for Crash Bash the scene record `0x8009F658`
packed with the active menu screen at `0x8009F8A4`, produced by `game/core/crashbash_input_phase.{h,cpp}`.
A phase-keyed file delivers each segment's masks relative to the guest entering that phase, so a
shorter handoff on the receiving build moves the segment boundary and the presses inside it with it.
That is what keeps `polar-push-control.pad` on its route after S020 collapsed the handoffs: the former
absolute file left the route on the S020 build and opened the LOAD GAME dialog instead. A segment the
guest never enters is dropped, so a recording cut on a screen it does not visit replays short and says
so (`inputFramesDropped` in the run's `padphase` line).

| replay | frames | route | Lightrec status |
|---|---|---|---|
| `crashball-control.pad` | 3,740 | attract exit → Cross menu → character select → **live Crashball match** → held Left (3560-3619) and held Right (3620-3739) | **requalified 2026-09-30 on the dynarec product** — live match reached, direction-driven movement measured against an idle control leg, 0 fallback |
| `battle-crate-crush-control.pad` | 15,401 | Battle Mode → live Crate Crush match | not yet requalified (S016 item 4) |
| `tournament-crate-crush-control.pad` | 4,330 | Tournament → 1P → character → first tournament → options → briefing (4 Cross) → **live Crate Crush match** (DAT28382) → held Left 60, Right 120 | **re-recorded and requalified 2026-10-02 on the dynarec product** |
| `polar-push-control.pad` | 1,394 | SELECT GAME TYPE Down → Battle Mode → 1P → character select ×3 → CHOOSE LEVEL Left then Right (onto Polar Push) → VS BATTLE → options → briefing (4 Cross) → **live Polar Push match** (DAT22510) → held Left 45 between briefing pages, held Right 120 after the match answers | **phase-keyed v1, recorded 2026-10-02** — 13 segments; replays to the live match with 1,394 of 1,394 frames delivered and moves P1 under Left on both the pre-S020 and the S020 build |
| `pogo-painter-control.pad` | 4,655 | Battle → 1P → character → CHOOSE LEVEL Right ×2 → VS BATTLE → options → briefing → **live Pogo Painter match** (DAT28272) → held Right 30, Up 30, Left 60 | **recorded 2026-10-02 on the dynarec product** |

`crashball-control.pad` is the one scenario whose input shape is non-obvious and worth stating: the
opening Cross taps (every 32 frames through 1192) only leave the attract cycle and reach the menu.
The route into the match needs the **800-frame Circle hold at 1200-1999**; tapping Cross alone reaches
character select and then sits there indefinitely, because on that stage Cross cycles the roster
instead of confirming. Cross at 2000 starts the match, Cross at 2560 leaves the briefing, and the
directions at 3560+ are the control inputs.

Measured evidence for the requalification — denominators, addresses, the control leg, frame-time
percentiles and the exact store that remains unattributed — is in
`docs/project-state.md` (S009, S016) and `docs/issues/0032-first-nested-module-had-no-image-identity.md`.
