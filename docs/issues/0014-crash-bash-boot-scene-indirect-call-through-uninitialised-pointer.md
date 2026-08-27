---
id: 14
title: The BOOT scene indirect-calls an uninitialised function pointer around frame 256
status: investigating
symptom: A 600-frame run aborts on [recomp-MISS] for 0x80070000, an address that is all zeros in both the retail executable and live RAM
state_items: S002,S004
tags: boot,scene,indirect-call,recomp-miss,menu-module
created: 2026-08-27
updated: 2026-08-27
---

## Where the game actually is

Measured with the frame driver's own progress diagnostic plus store watchpoints on each level of the
nesting. The port drives four nested machines, and the outer three are healthy:

1. **Process state.** Exactly one state is ever entered, `0x8004E0B8`
   (enter `0x80010410`, update `0x80010394`, present `0x80010278`). A store watchpoint on the
   process-state word `0x8005B648` records only two stores in 60 frames, both from the port's own
   `enterProcessState`, both writing the same value. **No guest code ever writes it.** That is
   correct rather than stuck: this is the application shell, and the retail runner `0x800270F0`
   loops on it until a handler requests a different state.
2. **App mode.** `0x8004E0DC` is set once to `0x80078C90` — the BOOT overlay base — with handlers
   `enter=0x80092BDC update=0x80092BA0 present=0x80092B7C`, and never changes.
3. **Scene.** `0x8009F658` does progress: cleared, then `0x800A00DC` (a BOOT-overlay scene), then
   `0x800B9524`, which is inside the loaded MENU module (`0x800B32B4..0x800BB2B4`). It settles
   there. The resident scene runner is `FUN_8001E610`, whose call site `0x8001E7C0` is the measured
   MENU-entry caller already recorded at the boot boundary.

So the game reaches and runs its MENU scene, calling update and present every frame with two display
fields delivered and the VBlank counter advancing by two per frame.

`empty prims`, printed once during startup, is the game's own stdout, not a framework message.
`FUN_8001888C` decompiles to a prim-pool teardown that frees each registered object's four prim
lists — a normal reset, not an error.

## The defect

A 200-frame run is clean and exits 0. A 600-frame run aborts:

```
[recomp-MISS 0] no recompiled fn for 0x80070000  (caller ra=0x800BBA60, a0=0x80051640, c->pc=0x800B5A40)
```

live chain `0x80092BA0 -> 0x8001E610 -> ov_boot 0x80093BE8 -> 0x80093588 -> 0x8001231C -> miss`.

**This is not a missing seed.** `0x80070000` is all zeros in the live RAM dump AND at the
corresponding offset of the retail executable (file offset `0x60800`), so it is BSS inside the
declared text extent and nothing is mis-loaded. The reported `ra=0x800BBA60` lies past the end of the
loaded MENU image and disassembles as data (`lb $ra,0x3ac8($zero)` / `mfc0`), not code. The guest
therefore dispatched through a function pointer that was never initialised, and the return address is
garbage too.

`c->pc=0x800B5A40` is a real function entry inside the MENU module (`addiu sp,sp,-0x18`).

## Ruled out

- Missing recompiler root. The distinct earlier miss on this path WAS one and is fixed; see below.
- A mis-sized or mis-placed MENU/BOOT module load: the zeros are present in the retail executable
  itself, so guest RAM matches the image.
- The card state machine. `FUN_8002C97C` oscillates between states 3 and 4 forever, but that is its
  normal idle poll: `DAT_800655C4` (the card REQUEST variable) is written once to 0 at init and never
  again, and the status word `DAT_80067830` settles to 0. Nothing is waiting on it.

## Fixed on the way here

The first miss on this path was a genuine missing root and met the evidence gate in
`game/recomp_seeds.json`: `[recomp-MISS 0] no recompiled fn for 0x80012678, caller ra=0x80012428`,
where the retail instruction at `0x80012420` is `jalr $v0` (invisible to static discovery) and
`0x80012678` is a real entry (`addiu sp,sp,-0x38`, then `sw s7/ra/s6..s0`). Seeding it took the
substrate from 2,005 to 2,006 functions and moved the run past that point to the miss above.

## Next step

Identify which object supplies the pointer dispatched near `0x800B5A40`, and what should have
initialised it. Do not seed `0x80070000`: it is not a function, and adding it would convert a real
uninitialised-pointer bug into a silent jump to zeros.
