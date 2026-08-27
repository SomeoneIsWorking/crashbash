---
id: 14
title: A direct call into the nested MENU range was bound to BOOT's own stale body
status: resolved
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

## Root cause

The uninitialised pointer was a SYMPTOM. The port was executing the wrong module's code.

`0x800B5A40` lies inside BOTH modules: the MENU image `[0x800B32B4,0x800BB2B4)` is nested inside the
BOOT image `[0x80078C90,0x800D7490)`. BOOT contains a real direct `jal 0x800B5A40` at `0x80096130`.
The emitter's `call_or_dispatch` bound any target in the emitting module's own function set
statically, so that call became `ov_boot_func_800B5A40` — BOOT's own body for that address. Once MENU
was loaded over the region, the RAM there held MENU code while the port kept running BOOT's stale
bytes. Those stale bytes then read a pointer out of a structure that only makes sense in the other
module's layout and dispatched through it, which is why both the reported target (`0x80070000`) and
the return address were garbage.

The backtrace is what proved it: there is no `rec_dispatch` frame between `ov_boot_gen_8009608C` and
`ov_boot_func_800B5A40`, so the call was static, not routed. The runtime router was never wrong — it
already resolves an address to the narrowest RESIDENT overlay — it was simply never consulted.

An earlier hypothesis, that the title's synchronous file-read owner fails to call
`overlay_note_load`, was FALSIFIED by comparing both baked signatures against the live RAM dump at
the failure point: MENU's signature matched, so MENU was correctly identified as resident all along.

## Fix

`tools/recomp/emit.py` (framework): a direct call may no longer be bound statically when the target
also lies inside a NARROWER overlapping module's range, because at runtime the resident module owns
those bytes and only the router knows which module that is. The emitter now computes, per module, the
set of fixed module ranges that overlap it and are narrower than it (`g_shadow`), and routes calls
into them. Equal-width overlaps are excluded: those are alternative modules sharing one slot, which
the router already distinguishes by signature and where neither shadows the other. Overlay images are
now loaded before MAIN is emitted so MAIN gets the same treatment — its declared text runs to
`0x80079000` while BOOT loads at `0x80078C90`, and two MAIN functions sit in that overlap.

`RECOMP_VERSION` is bumped to `2026-08-27.1` so a stale `generated/` is detected.

## Evidence

After regeneration both call sites emit `rec_dispatch(c, 0x800B5A40u)` instead of the static call, and
MAIN's calls to `0x80078CA4` / `0x80078DD4` likewise route. The 600-frame run no longer produces any
`[recomp-MISS]`. Gates: 12/12 CTest, `verify_boot.py --run` PASS, and a clean 200-frame run (exit 0,
no watchdog, fatal, recomp miss, or guest VSync violation).

## What it exposed next

With the correct module now executing, the 600-frame run reaches a NEW first-time guest VSync site and
fail-fasts as designed: `GUEST VSYNC VIOLATION: reached 0x800320EC a0=1 ra=0x8008BB88`, via
`ov_boot 0x8007976C -> ov_boot 0x8008BB48`. That is the next top-down owner, tracked in issue 0012 —
not a regression from this fix.
