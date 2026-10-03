---
id: 31
title: Crash Bash's retail game cadence is 2 fields per game frame, set by a literal in application main
status: open
symptom: S006 says interpolation preserves "the retail simulation cadence" but no document in this repository records whether that cadence is 30 or 60 fields per second
state_items: S006
tags: cadence,vsync,frame-rate,presentation,interpolation,tooling
created: 2026-09-27
updated: 2026-09-27
---

## Answer

**2 fields per game frame — 30 fields/s, ~30 game frames/s.** Not 60.

The cadence is not negotiated at runtime from a menu or a hardware probe. It is a
**compile-time literal `2`** written into a global by application main, and the game
asks `VSync` for that many fields once per present.

## The rate-selection global

`0x8004E0E0` — already named in the port as `guest::kDisplayFieldsPerFrame`
(`game/core/crashbash_guest.h:51`). It is **zero in the image** (a PS-X EXE stores no
separate initialized-data section; the file is header + text and `0x8004E0E0` reads
`00000000` at file offset `0x3E8E0`), so the value is established by code, not by a
data constant.

Complete census, so the count has a denominator: every instruction whose `imm16` is
`0xE0E0` (i.e. `-0x1F20`) paired with a `lui $reg, 0x8005` within 12 words, over
**323,072 words across all 8 linked images** — resident plus BOOT, MENU and the five
DAT gameplay overlays. **7 sites total: 5 writers, 2 readers.** The six gameplay/MENU
overlays contain **0**.

| site | word | what it is |
|---|---|---|
| `0x80010190` | `0xAC62E0E0` | writer, resident, application main |
| `0x80010328` | `0xAC62E0E0` | writer, resident, second application-init copy |
| `0x8008E918` | `0xAC50E0E0` | writer, BOOT, guarded by `beq $s0, 2` |
| `0x80094EA4` | `0xAC83E0E0` | writer, BOOT, literal 2 |
| `0x80094694` | `0xAC46E0E0` | writer, BOOT, stores **3** — see the gap below |
| `0x800102CC` | `0x8C44E0E0` | **reader** |
| `0x80010378` | `0x8C44E0E0` | **reader** |

Four of the five writers store the literal 2, in the same two instruction encodings:

```
8001017C  lui   $v1, 0x8005        0x3C038005
80010180  addiu $v0, $zero, 2      0x24020002
...
8001018C  jal   0x80011bf8         0x0C0046FE
80010190  sw    $v0, -0x1f20($v1)  0xAC62E0E0   -> 0x8004E0E0 = 2
```

(0x80010180 / 0x80010300 are both `0x24020002`; 0x80010190 / 0x80010328 are both
`0xAC62E0E0`. BOOT's `0x80094EA0` is `0x24030002` — `addiu $v1, $zero, 2` — and
`0x80094EA4` is `0xAC83E0E0`, the same store through `$v1`.) The store is in the
**delay slot** of the following `jal`, which is why `crashbash_boot.cpp:45` reproduces
it as a native `mem_w32` rather than as a call.

The fifth writer is BOOT `0x8008E918`, and it cannot store anything else — the only
transfer into that block is the `beq` that tests the same value against 2:

```
8008E744  lw    $s0, ($v1)         0x8C700000
8008E748  addiu $v0, $zero, 1      0x24020001
8008E74C  beq   $s0, $v0, 0x8008E764
8008E750  addiu $v0, $zero, 2      0x24020002
8008E754  beq   $s0, $v0, 0x8008E8B4   0x12020057
...
8008E910  lui   $v0, 0x8005        0x3C028005
8008E914  jal   0x8001fe80
8008E918  sw    $s0, -0x1f20($v0)  0xAC50E0E0   -> 0x8004E0E0 = 2
```

## The two readers feed the display owner directly

Both readers sit in a process-state `present` handler and pass the value as `a0`
straight into the display owner. There is no scaling, no compare, no clamp:

```
800102C8  lui $v0, 0x8005        0x3C028005
800102CC  lw  $a0, -0x1f20($v0)  0x8C44E0E0
800102D0  jal 0x800272ac         0x0C009CAB
```

and identically at `0x80010374` / `0x80010378` / `0x8001037C`. The process struct at
`0x8004E0B8` holds `0x80010410` / `0x80010394` / `0x80010278` at `+0` / `+4` / `+8`, so
`0x80010278` is `kInitialStatePresent`, and `0x80010354` is the sibling state's present.

## The display owner turns the parameter into the VSync wait

`0x800272AC` (`guest::kDisplayFrame`) preserves the incoming argument into `$s2` and
re-uses it as the VSync argument:

```
800272B4  move $s2, $a0        0x00809021   ; s2 = incoming a0 = the field count
...
80027378  jal  0x800320ec      0x0C00C83B   ; VSync(1)  -- sub-frame sample, no wait
8002737C  addiu $a0, $zero, 1  0x24040001
80027380  move $a0, $s2        0x02402021   ; a0 = the field count again
80027388  jal  0x800320ec      0x0C00C83B   ; <-- THE PACING WAIT
```

`0x800320EC` is what makes the argument a **field count** rather than a "wait once"
flag, and this is the decisive shape:

```
8003215C  addiu $v0, $zero, 1                0x24020001
80032160  beq   $a0, $v0, 0x8003224C         0x1082003A   ; a0 == 1 -> NO WAIT (sub-frame timer)
80032168  blez  $a0, 0x80032188              0x18800007   ; a0 <= 0 -> NO WAIT (return vblank counter)
80032170  lui   $v0, 0x8007
80032174  lw    $v0, -0x7438($v0)            0x8C428BC8   ; 0x80068BC8 = current counter
8003217C  addiu $v0, $v0, -1                 0x2442FFFF
80032184  addu  $v0, $v0, $a0                0x00441021   ; target = current + a0 - 1
80032198  addiu $a1, $a0, -1                 0x2485FFFF
8003219C  jal   0x80032264                   0x0C00C899   ; spin until the counter reaches target
```

So `a0 == 1` explicitly does **not** wait, and `a0 <= 0` explicitly does **not** wait.
Only `a0 >= 2` waits, and it waits for `a0 - 1` further fields — which with the
in-progress field is `a0` fields per call. With `a0 = 2`: one more field boundary per
present.

The wait primitive polls a genuine field counter. `0x80032264` reads
`*(0x8006D8DC)` (`kVblankCounter`) and compares it against the target, and the VBlank
root increments that word by exactly one per interrupt:

```
8003ADD8  lw   $v0, -0x2724($v0)   0x8C42D8DC   ; 0x8006D8DC
8003ADF8  addiu $v0, $v0, 1        0x24420001
8003AE00  sw   $v0, -0x2724($at)   0xAC22D8DC   ; exactly +1 per VBlank = per FIELD
```

PSX NTSC is ~59.94 fields/s, so 2 fields per frame is ~30 game frames/s.

## Second, independent corroborating site: the process runner

`0x800270F0` is the non-returning process runner. Its loop is a bare
`{ update; present }` back-edge with **no counter, no parity test, and no comparison
of any kind**:

```
80027134  lw   $v0, 4($s0)    0x8E020004   ; update handler
8002713C  jalr $v0
80027144  lw   $v0, 8($s0)    0x8E020008   ; present handler
8002714C  jalr $v0
8002715C  beq  $v0, $s0, 0x80027134   0x1050FFF5
```

So there is exactly **one present per game frame**, and one present is exactly one
`VSync(2)`. The "waits twice" shape is a single `VSync(2)`, not two `VSync(1)` calls —
and it could not be two `VSync(1)`s, because `VSync(1)` is the no-wait sub-frame query.

## Third site: the VSync call census

Of **54** `jal 0x800320EC` sites in the linked images (47 resident + 4 BOOT + 3
DAT22510 — this reproduces the counts in `docs/findings/vsync-owner-map.md` exactly),
**exactly one** passes an argument that is neither a literal `<= 0`, nor the literal
`1`, nor a small CD/GPU timeout constant: `0x80027388`, with `a0 = $s2`. Every other
site is a non-waiting query or a timeout clock. There is no second candidate that
could be a one-field pacing wait.

## What the opposite answer would have looked like, and why it is absent

A 60-field cadence would have needed one of:

1. **The global set to 1.** Structurally impossible on this path: `VSync(1)` takes the
   explicit no-wait branch at `0x80032160`, so a one-field request would not pace
   anything. The game cannot ask for 60 through this primitive.
2. **A "skip every other VBlank" shape** — a frame counter compared against 1, or a
   parity test on a monotonic counter. Absent: the process runner's loop
   (`0x80027134`-`0x80027160`) has no counter and no comparison at all.
3. **A VSync site taking `a0 = 1` as a wait.** Absent: see the census above — the
   single pacing site is the only one with a non-literal, non-query argument.
4. **A boot-time 30-vs-60 selector.** The four 2-valued writers are literals; the
   fifth is guarded by a compare against 2. No writer anywhere computes a rate.

The port's own model already agrees: `crashbash_boot.cpp:45` writes 2, and
`crashbash_frame_driver.cpp:51-77` loops `fields` times, dispatching the VBlank root
each time and aborting unless `0x8006D8DC` advances by exactly 1 per field.

## Gap: BOOT `0x80094694` stores 3, and I did not resolve when it runs

The fifth writer is **not** a 2:

```
800945E0  beqz $v0, 0x80094608    0x10400009
800945E4  addiu $a2, $zero, 3     0x24060003   ; delay slot, always executes
80094608  addiu $v0, $zero, 0x3c  ; 60
8009460C  div   $zero, $v0, $a2   ; 60/3 = 20, inside a GTE/fixed-point transform
...
80094690  lui   $v0, 0x8005       0x3C028005
80094694  sw    $a2, -0x1f20($v0) 0xAC46E0E0  -> 0x8004E0E0 = 3
```

`$a2` is 3 there unconditionally: `0x80094608` has exactly one transfer into it
(the `beqz`, whose delay slot always runs) and no other `$a2` definer lies between
`0x800945E4` and the store. The surrounding block is a signed fixed-point divide
(`mult` by `0x88888889`, `mfhi`, `sra 5`), and I read the store as an oddity inside
it — I have **not** established whether that path runs per frame or is one-time
initialization, nor whether it can be reached while BOOT is the active scene.

This does not change the answer for gameplay: all five gameplay/MENU overlays contain
**zero** accesses to the global, and the resident re-asserts 2 at every application and
scene start, so nothing loaded for a match can change the value a present handler
reads. It is recorded because "four writers say 2, one says 3" is not the same
statement as "the cadence is 2", and the honest denominator is five writers.

## Tooling defect found on the way: `tools/probe_call_sites.py` cannot match any address in this game

`tools/probe_call_sites.py` computes a JAL target as `(word & 0x03FFFFFF) << 2` with
**no region term**. The correct MIPS form is
`((pc + 4) & 0xF0000000) | ((word & 0x03FFFFFF) << 2)`. Every code address in these
images is `0x8xxxxxxx`, so the tool's targets are bounded by `0x0FFFFFFC` and
`--callee 0x800272AC` **can never match anything** — it printed `matched 0` over
107,520 scanned words, which is a guaranteed answer, not evidence.

The tool demonstrates the bug itself:

    $ python3 tools/probe_call_sites.py --root scratch/bin/crashbash --image SCUS_945.70 --caller 0x80027378
    [probe] SCUS_945.70: scanned 107520 word(s) at 0x80010000..0x80079000, decoded 3013 jal, matched 1
        0x000320EC          <-- should read 0x800320EC

`--caller` mode is wrong for every target it prints, for the same reason. I read the
`matched 0` as "the display owner has no JAL callers" and had to redo the question;
the real answer is that it is reached only by `jalr` through the process table, which
is why `--word` mode exists.

This does **not** undermine `docs/findings/vsync-owner-map.md`: its 47-resident and
4-BOOT counts reproduce exactly under the corrected formula, so that finding used a
different and correct method. Fixing the region term is a separate change and is not
made here.

## Method note

`scratch/` (gitignored) holds the instruments, all word-level field tests in the
style of `tools/probe_call_sites.py` rather than a new disassembler; every site quoted
above was re-read with the workspace's Ghidra pipeline
(`external/psxport/tools/decomp_pipeline.py --image <exe> --function-at 0xADDR`):

- `scratch/cadence/scan_field_global.py` — the `0xE0E0` census, with the lookback
  window and its match count reported.
- `scratch/cadence/census_fields_and_vsync.py` — both censuses across all 8 images,
  with the corrected JAL target and `$a0` resolved from the nearest definer.
- `scratch/cadence/find_definer.py` — most-recent definers of a register before a site.
- `scratch/cadence/branch_targets.py` — control transfers into an address window, with
  a **positive control** target so a zero count can be read as a count. This is what
  caught my own first branch-target formula being wrong: the control for `0x8008EA64`
  returned 1 of the 2 visible transfers, and 2 of 2 once the sign-extended offset form
  was used for `beq`.
- `scratch/cadence/view/*.view.exe` — overlays are raw payloads with no PS-X EXE header,
  so a PS-X EXE reader cannot read them. Each view is the module's own bytes
  verbatim behind a synthetic 0x800-byte header carrying the load address from
  `titles/crashbash/<name>_module.json`. No guest byte was altered; the provisioned
  inputs under `scratch/bin/` were not touched.
