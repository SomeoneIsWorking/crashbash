---
id: 32
title: The first nested gameplay module had no image identity at all, which is where the product actually stopped
status: open
symptom: docs/project-state.md records the product reaching only f255 in one process state; a longer
  run reaches the attract flow, loads DAT28272, and then the executor refuses guest address
  0x800C3434 as "ambiguous code-image identity" and the run ends
state_items: S003, S004, S008
tags: image-identity,module-publication,cd-file-read,frontier,probe
created: 2026-09-27
updated: 2026-09-27
---

## The recorded frontier was a budget artefact, not the boundary

`docs/project-state.md` records a 400-frame run ending at f255, dwelling in `0x8004E0B8` for 256
frames, and correctly says "reaches f255 is NOT progress". The frame budget was the limit, not the
product. Raised to 1500 frames the same build completes the boot logo sequence and keeps going; the
debug server, which lifts the cap by design (`native_boot.cpp`: "When the debug server is up
(headless, no REPL), the run is INTERACTIVELY DRIVEN over the socket — do NOT cap it"), carried one
run to 37,506 presented frames and 21 authenticated image generations.

## The real boundary, and it is a correct refusal

    [native-dispatch:error] guest address 0x800C3434 resolves to zero or multiple active code images
    [executor:error] Crash Bash process update required a completed guest call, but execution
      exited as fault at 0x800C3434 after 251040 cycles: ambiguous code-image identity

The refusal was right. `crashbash-cd native file read completed: 38 sector(s) from LBA 28272 to
0x800B32B4` is `titles/crashbash/dat28272_module.json` exactly (`payload_disc_lba` 28272,
`sector_count` 38, `load_address` 0x800B32B4), and `cdFileReadOwned` had already done the correct
thing with it: `retireImagesForCdSectorWrite` removed BOOT's authenticated coverage over the bytes
it rewrote. Nothing then published the new module, so the range had **zero** image identity and
`resolveHostDispatch` step 3 turned it into a typed fault. Two facts made the gap:

- `runtime::GuestImage` had no value for DAT28272, DAT28241 or DAT28382, so a native key for them
  could not even be named.
- `cdFileReadOwned` offered each read only to `completeBootImageRead` / `completeMenuImageRead`.
  The codemap had already flagged this: "Later nested modules belong at this title-owned
  publication boundary."

## 0x800C3434 is the module's initializer, read from the image

It is inside DAT28272's payload range `0x800B32B4..0x800C62B4` and opens by installing its own
behaviour table:

    800C3434  addiu $sp, $sp, -0x18
    800C3438  lui   $v1, 0x8006
    800C343C  lui   $v0, 0x800c
    800C3444  sw    $v0, -0x5590($v1)      ; 0x8005AA70 — the address docs/project-state.md
                                                 already attributes to DAT28272

BOOT also stores the word `0x800C3434` at `0x80098FEC`, so the resident module table reaches the
initializer directly.

## The entry-word witness does not exist for this module family, and that is measured

`AuthenticatedModuleSpec` demanded a second content witness: the word at `entry_pointer_offset`
must equal `entry`. BOOT records offset 0 -> `0x80092BDC` and MENU 0x6270 -> `0x800B5244`. The five
nested overlays record `null` for both, and that is a property of the images: DAT28272's first eight
words decode as ASCII — `42 52 45 41 54 48 45 00` = "BREATH", `4A 55 4D 50` = "JUMP", `44 41 5A 45` =
"DAZE", `57 49 4E 00` = "WIN" — the animation-state name table `docs/project-state.md` already
describes for this module. Reading four bytes of a name table and comparing them against 0 would be
a guaranteed mismatch and a refusal that says nothing about the image.

So `hasEntryWitness(spec)` is now a stated property of the specification, `validModuleSpec` requires
the entry only when one was recorded, and both the success and the refusal log name how many
witnesses were used. The SHA-256 over the whole payload remains the content authority in every case.

## The fix

`game/core/nested_module_image.{h,cpp}` holds the five tracked nested specifications, built from the
same CMake-derived identity headers provisioning authenticates, and offers a completed read to the
ONE existing implementation `completeModuleImageRead`. No second digest, retirement, invalidation or
binding was written. `cdFileReadOwned` calls it beside the BOOT and MENU offers.
`GuestImage` gained `Dat28272`, `Dat28241`, `Dat28382`.

## Measured, after

| measurement | 4:3 | 16:9 |
|---|---|---|
| guest calls completed | 2,354,715 | 2,354,715 |
| needing a resume | 60 (deepest 7 turns, cap 12) | 60 |
| executor calls | 2,497,655 | 2,497,655 |
| executed blocks | 304,095,272 | 304,095,272 |
| executed instructions | 4,740,055,996 | 4,740,055,996 |
| fallback blocks / instructions | **0 / 0** | **0 / 0** |
| faults | 0 | 0 |
| image generations published | 16 (boot 1, menu 10, dat28272 3, dat28241 2) | same |

The two aspects produce **identical** guest-execution telemetry, which is the property widescreen is
supposed to have: the picture changes, the simulation does not.

Live census from the long run's debug endpoint (37,506 presented frames, a different run from the
capped pair, quoted because it is further along):

    calls=4382380 translated_blocks=10584 executed_blocks=540076720
    executed_instructions=8442308567 host_dispatches=4209650
    cache_hits=540061054 cache_misses=15669 faults=0
    fallback: calls=0 instructions=0 refused_calls=0, and every reason 0

Zero interpreter fallback by any of `compilation_failed`, `self_modifying_code`,
`unsupported_block`, `load_delay_hazard`, `unsafe_instruction_fetch`. Gameplay execution is
nonzero-dynarec, so this is not interpreter-covered evidence.

## Gap

- The nested modules now have identity, but the product still never leaves process state
  `0x8004E0B8`: `kAppModeVtable` (`0x8004E0DC`) is written by exactly ONE instruction across all 323,072
  words of the 8 linked images (`0x800101CC`, resident application main), and the app-mode object IS
  the loaded module's own header (BOOT+0/+4/+8 = its enter/update/present). Nothing re-points it.
  The BOOT logo handoff is armed and measured — `0x8008E5BC` writes `0x8009F65C = 0x800A00DC` with
  flags `0x12` on table exhaustion, and the 4-word transition clock at `0x8009F644`
  (value/step/age/flags) satisfies `0x8001E610`'s gate after about three traverses — but the observed
  frontier is still the attract flow reloading MENU and DAT28241, not a controlled game mode.
- The gameplay-reads hazard for this title's widening is **NOT** answered, and cannot be answered by
  an absolute-address scan: the projection H the model producer widens is `camera + 0x18` where
  `camera = *0x800569E0`, and the horizontal scale is `*0x8005B698 + 4`. Both sit behind pointer
  globals, so neither has an address to scan. What is measured is that each pointer has exactly 2
  writers, all resident (`0x80018C3C`, `0x80029C94` and `0x80027590`, `0x8002768C`). Answering the
  hazard needs a runtime store/branch observation on the camera struct, which has not been done.
  Do not read the pair below as "safe to widen": it is a picture result, not a scalar-safety result.
