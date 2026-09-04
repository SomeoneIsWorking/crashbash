# Retail render-anchor inventory

This document preserves the address-level results of the retired generated-source analyzer. The
analyzer and its emitted inputs were deleted during the break-first dynarec migration; they are not a
current toolchain or oracle. The recorded projection anchors combined a GTE operation with projected
screen/depth consumption, while the camera-control set recorded writes to GTE control registers 0..7
and 24..26. These are RE entry points, not native producers and not proof of a visible submission.

## Exact provenance answers

The former analyzer recorded provenance for two distinct answers:

| psxport | Recompiler | Generated functions | Projection anchors | Camera-control anchors |
|---|---|---:|---:|---:|
| previous verified pin `17981527` | `2026-08-24.2` | 1,724 | 31 | 17 |
| substrate baseline `99a42aa3` | `2026-08-26.14` | 2,005 | 31 | 17 |

The module denominators are likewise provenance-bound:

| psxport | Resident functions/anchors/control | BOOT functions/anchors/control | MENU functions/anchors/control |
|---|---:|---:|---:|
| `17981527` | 941 / 10 / 15 | 696 / 21 / 2 | 87 / 0 / 0 |
| `99a42aa3` | 1,083 / 10 / 15 | 823 / 21 / 2 | 99 / 0 / 0 |

Equal aggregate anchor counts do not mean the answer is identical. The newer recompiler replaces
BOOT projection entries `0x800A0014` and `0x800BBA7C` with `0x800AEC58` and `0x800BCCF4`, and the
resident camera-control entry `0x80032F3C` with `0x80032F5C`. Those differences follow changed
function boundaries. Runtime attribution must therefore name the exact game/framework provenance;
it cannot reuse an address list from a different generated substrate.

The strongest resident projection chain is stable across both exact answers:

- `0x80015780` directly calls `0x8001CD04`.
- `0x8001CD04` writes GTE rotation/translation/screen-offset registers 0..7 and 24..25, executes
  five GTE operations, stores four projected screen coordinates, and directly calls both
  `0x800193A8` and `0x8001AF2C`.
- `0x800193A8` executes eight GTE operations and stores six screen coordinates.
- `0x8001AF2C` executes thirteen GTE operations, stores eighteen screen coordinates, and reads
  eighteen projected depths. Its other direct caller is `0x80019A60`.

The nearest projection-scale sites are also stable across both answers. Resident `0x80018B08` and `0x8002AEE0` write
the full control set 0..7 and 24..26; BOOT `0x8009440C` writes register 26 and has direct BOOT callers
`0x8008EB3C`, `0x8009414C`, and `0x80094EB8`. Static output alone does not establish which is the
active gameplay camera or whether any one is merely a PSYQ-style matrix helper.

## Next falsifiers

- **Native renderer:** after the clean CDC/completion gate reaches the first submitted frame, capture
  runtime GTE/OT attribution for the addresses above. A frame whose submitter ancestry excludes this
  inventory falsifies the proposed first RE slice; decompile the observed ancestry instead. Native
  producers must read decoded game state and must not replay GTE, OT, or GP0 output.
- **Widescreen:** prove the active camera/projection-state owner and the game-state input that reaches
  GTE register 26 or an equivalent projection lens. The first native gate is a 4:3 parity frame;
  only then may a wider aspect expand horizontal view without changing vertical framing. Changing a
  GTE/GP0 scalar is not widescreen ownership.
- **Interpolation:** identify two consecutive simulation-owned camera/world transform snapshots and
  their submitter consumers. The first native gate is bit-identical simulation with interpolation
  disabled, followed by a midpoint render at alpha 0.5 that does not mutate guest RAM. Interpolating
  projected screen coordinates or guest GTE matrices is not accepted.

New attribution must be reproduced from the authenticated binary or observed through the dynarec;
the deleted emitter path must not be restored to reproduce this inventory.
