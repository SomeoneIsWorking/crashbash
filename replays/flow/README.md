# `replays/flow/` — representative gameplay scenarios

One active-low PSX pad mask per host frame, little-endian `uint16`, `0xFFFF` = every button released.
They are the minimum retained scenarios for requalifying a title after the static execution path was
retired, not claims that every retail mode is covered. Feed one with
`PSXPORT_PAD_REPLAY=<path> PSXPORT_NATIVE_FRAMES=<frames>`; the guest receives the masks through the
real SIO0 pad chain, so every transition below is the guest's own.

| replay | frames | route | Lightrec status |
|---|---|---|---|
| `crashball-control.pad` | 3,740 | attract exit → Cross menu → character select → **live Crashball match** → held Left (3560-3619) and held Right (3620-3739) | **requalified 2026-09-30 on the dynarec product** — live match reached, direction-driven movement measured against an idle control leg, 0 fallback |
| `battle-crate-crush-control.pad` | 15,401 | Battle Mode → live Crate Crush match | not yet requalified (S016 item 4) |
| `tournament-crate-crush-control.pad` | 7,910 | Tournament Mode → first Crate Crush match | not yet requalified (S016 item 4) |
| `polar-push-control.pad` | 17,682 | Polar Push → a complete, controllable live match | not yet requalified (S016 item 4) |

`crashball-control.pad` is the one scenario whose input shape is non-obvious and worth stating: the
opening Cross taps (every 32 frames through 1192) only leave the attract cycle and reach the menu.
The route into the match needs the **800-frame Circle hold at 1200-1999**; tapping Cross alone reaches
character select and then sits there indefinitely, because on that stage Cross cycles the roster
instead of confirming. Cross at 2000 starts the match, Cross at 2560 leaves the briefing, and the
directions at 3560+ are the control inputs.

Measured evidence for the requalification — denominators, addresses, the control leg, frame-time
percentiles and the exact store that remains unattributed — is in
`docs/project-state.md` (S009, S016) and `docs/issues/0032-first-nested-module-had-no-image-identity.md`.
