---
id: 11
title: Crash Bash projection anchors lack runtime submitter and camera-state attribution
status: investigating
symptom: The verified generated substrate contains projection/control sites, but no PC-owned producer can yet identify which retail camera and submitter state produced a visible frame
state_items: S004,S005,S006
tags: graphics,re,camera,native-renderer,widescreen,interpolation
created: 2026-08-26
updated: 2026-08-27
---

## Finding

`tools/inventory_render_anchors.py` now refuses unbound generated caches. Exact recorded pin
previous pin `17981527` has 1,724 generated functions and substrate baseline `99a42aa3` has 2,005;
both contain
31 projection anchors and 17 camera-control anchors, while three individual addresses differ because
the recompiler changed function boundaries. The strongest resident chain remains
`0x80015780 -> 0x8001CD04 -> {0x800193A8, 0x8001AF2C}` in both. This bounds a static RE slice but does
not prove runtime execution, camera semantics, primitive submission, or a native producer.

## Exact-pin runtime attempt

The operator's clean `99a42aa3` `rtpcaller` run reached resident, BOOT, and MENU code and printed
`empty prims`, but presented zero game frames before the 30-second watchdog exited 134. The watchdog
backtrace includes `gen_func_8002DE2C`, `ov_boot_gen_8008E5BC`, and
`ov_menu_gen_800B5218`; it does not include a completed 50-present `rtpcaller` histogram.
`tools/verify_render_anchor_reach.py` therefore correctly refused the exact local log at
`scratch/logs/crashbash-render-anchor-reach.log` because `[watchdog] STUCK` is forbidden. This is a
live falsifier of the current first-frame/reachability gate, not projection-anchor attribution.

## Proper resolution

First use the process-driving CDC gate with `--port`, `--executable`, and `--timeout` to identify the
post-MENU no-present cause; do not pass it a positional log (only `--trace PATH` judges a saved CDC
trace). After issues 0007/0009 pass that clean serialized CDC/completion gate, run the product with
`PSXPORT_DEBUG=rtpcaller` through at least 50 presented frames and feed the log to
`tools/verify_render_anchor_reach.py`. The judge binds every decoded projection target to the exact
generated inventory and refuses zero/unknown ancestry; its 8/8 selftest proves those opposite
answers. Decompile the observed ancestry back to decoded game camera/object/material state. Implement
native producers from that state, first proving 4:3 parity. Then identify two simulation snapshots
for interpolation and widen the native projection without changing vertical framing. Replaying or
patching GTE, OT, or GP0 output is explicitly rejected.

## Current clean-product A/B and semantic boundary

The clean `0d4712f` product was rebuilt with Clang 22.1.8 from the verified USA disc against
psxport `3a8256e9`, then run for 600 native-owned frames on each render path. Both runs returned
from native crt0 with no guest-VSync violation, timeout, watchdog, recompilation miss, or fatal.
The shipping native path produced three 960x720 captures with 0/691200 non-black pixels. The PSX
diagnostic path produced 642043, 683756, and 577480 non-black pixels at frames 300, 450, and 599;
visual inspection shows the Eurocom and Crash Bash title screens. This proves the simulation and
retail submitters are live while the shipping picture owner is absent.

Ghidra decompilation of the live-positive chain establishes a pre-GTE ownership boundary:

- `0x80019F1C` is the standard object-level model submitter. It accepts an object, explicit matrix,
  translation (`matrix + 0x14`), and call flags, and rejects objects without frame, render-enabled
  flag `0x8000`, model asset (`object + 0x6C`), or model data (`asset + 0x0C`).
- `0x8001DD50` is the alternate object-level model submitter over the same object/model contract.
- `0x8001965C` composes object and camera transforms before installing them in the GTE.
- `0x8001C1E0` decodes source vertices and `0x8001C0F0` interpolates two source vertex streams into
  the retail work buffer.
- `0x8001AF2C` performs projected model-bounds culling, and `0x800193A8` finally links textured
  polygons into the guest ordering table. Those last two are locators/oracles, not legal native
  picture inputs.

The current working candidate wraps only `0x80019F1C` and `0x8001DD50`, retains both generated
supers, and captures the accepted object/model/transform inputs into a two-frame title-owned
`SceneSnapshotHistory`. It reads no GTE result, ordering table, or GP0 packet. The candidate is an
input boundary for later native producers and interpolation; it does not yet claim a native picture.
A 350-frame real-disc run reaches the standard capture 17,004 times and the alternate capture zero
times, completes all frame fences, and returns from native crt0 without a guest-VSync violation,
timeout, watchdog, recompilation miss, or fatal. A second run with the per-frame denominator reports
71 accepted pre-GTE model draws in frame 255 (10,452 standard-submitter calls over 300 frames), so
the capture is not merely reached: it publishes non-empty semantic snapshots. The history
rotation/storage selftest passes.

The next decoder boundary is also grounded. `0x80019A60` resolves the selected frame record, then
`0x8001C1E0` expands its indexed source vertices from six-byte signed XYZ records into an eight-byte
working record (XYZ plus the low two index bits); `0x8001C0F0` creates the same record while
interpolating two source streams. The grouped primitive stream begins at the selected render record's
`+0x14` relative target: a flag/count pair repeats until `0xFF`, and `0x800193A8` consumes the decoded
records while choosing its triangle/quad packet layout. Separately, `0x80017B08` consumes the model's
material-index stream and resolves the texture/color words that the guest packet allocator prepared.
Those source vertex/topology/material records are the next legal native input; the resulting packet
pool remains only an oracle.

## First source-owned 4:3 producer

The smallest verified path is the fixed-frame family (`frame & 0x7000 == 0x2000`) with untextured
Gouraud triangle strips (`topology flags & 1`). `model_recipe_capture.cpp` copies its eight-byte XYZ/
flag vertices and three source RGB words per face into the current `SceneSnapshot`; the producer then
uses psxport's pure fixed-affine native projection and the title's original 4:3 projection constants.
It reproduces the strip's two winding bits and submits through the native depth queue under guest
producer key `0x80019F1C`. Textured groups advance the same source cursors but are counted and refused,
not guessed.

The transform composer initially appeared unreached by the snapshot because its fifth-argument output
structure is at PSX scratchpad `0x1F8002D4`, outside main RAM. The generated `0x8001965C` super was in
fact reached 673 times by frame 255. Accepting the PSX's real 1 KiB scratchpad range lets the wrapper
copy its rotation pointer `0x80056A64`, translation pointer `0x80056AA4`, and projection pointer
`0x8005B72C`; no GTE control/output is read.

A serialized real-disc frame-255 A/B used exact native PID 3026346 followed by exact PSX PID 3026483.
Both returned from native crt0 with no guest-VSync violation, timeout, watchdog, recompilation miss,
or fatal. Both saw 71 accepted fixed models and 1,160 supported untextured source faces. The native leg
submitted 717 after winding/depth rejection and captured 627072/691200 non-black pixels (90.72%); the
PSX reference submitted no native faces and captured 668153/691200 (96.67%). Visual inspection shows
the native animated background and a corresponding green title-letter fragment. The PSX leg additionally
shows the textured letters and particle effects, so this is a real partial producer and not 4:3
completion. The next unit is fixed-frame textured UV/material decode before any widescreen or lerp.

## Fixed-frame textured source path

Retail `0x80017EE8` pre-fills each fixed-frame face from two source streams that advance across both
textured and untextured groups: one 16-bit UV-table index per face and a descriptor whose bits 9..14
encode a run-minus-one. Descriptor bit 15 selects one of two model-owned texture/CLUT table forms.
The resolved texture state supplies a UV base and texture page; the material supplies the page blend
override, color index, and semitransparency bit. `model_recipe_capture.cpp` now copies those resolved
values atomically into the snapshot. An invalid pointer or descriptor rejects the whole model recipe
rather than publishing a partial prefix.

A serialized frame-255 native run from the restored shipping binary used exact PID 3121483 and
returned without a guest-VSync
violation/timeout, watchdog, recompilation miss, or fatal. It captured 1,542 fixed faces, including
382 textured, and submitted 1,010 after winding/depth rejection. Its 960x720 capture is
627672/691200 non-black. A retained before/after comparison against the untextured producer shows the
same central green title fragment plus three newly sampled gradient squares and additional particle
points. The serialized PSX reference, exact PID 3090121, remains 668153/691200.

This is still not 4:3 parity. At frame 300 the native producer shows only central Eurocom-letter
fragments while the PSX reference shows the complete logo. A diagnostic substitution of
`RQ_OM_2D_FG` for the producer's world-depth order was rejected by the renderer's painter-plan
invariant (`refused=3`) and reverted; reclassifying 3D model work as HUD is not an ordering fix. The
next grounded unit is the source-owned sort/coverage behavior around `0x800193A8`, followed by any
remaining frame/submitter families. Widescreen and interpolation remain downstream of visual 4:3
correctness.
