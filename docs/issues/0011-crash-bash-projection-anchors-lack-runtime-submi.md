---
id: 11
title: Crash Bash projection anchors lack runtime submitter and camera-state attribution
status: investigating
symptom: The verified generated substrate contains projection/control sites, but no PC-owned producer can yet identify which retail camera and submitter state produced a visible frame
state_items: S004,S005,S006
tags: graphics,re,camera,native-renderer,widescreen,interpolation
created: 2026-08-26
updated: 2026-08-28
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

### Note (2026-08-27)
Static source boundary advanced on the unlanded fb08d30f candidate: generated `0x800193A8`
proves untextured-only `SZ3 == 0` rejection, unsigned `((SZ3 + signed depth bias) >> 1)`
bucket comparison against the signed halfword limit, and winding/no-cull behavior. The native producer
now supplies that bucket to the keyed world queue without `PainterObjectScope`. Retail `0x800159C4 ->
0x8001DD20` grounds family `0x5000` as a two-level source-frame resolver, while `0x8001D894` grounds
the alternate submitter transform from object/camera matrices and large signed translation. Clang
shipping build, 16/16 CTests, policy, and pin provenance pass. Serialized native PID 3368774 then
completed 301 frames and produced the complete EUROCOM letter geometry at frame 300, but the model
layer is visibly over-scaled/cropped and blacks out much of the starfield. This materially advances
coverage while falsifying 4:3 parity. Exact PID 3396467 then forced global Authored mode and completed
cleanly, but retained the same scale/crop at 266224/691200 non-black versus Depth's 266233/691200.
This falsifies cross-object OT policy as the cause. The retained PSX frame-300 image predates current
framework provenance, so a current-build PSX-path witness is required before changing projection.

### Note (2026-08-27, current-build PSX falsifier)
Exact PID 3410230 completed the authorized 301-frame PSX-path run cleanly at build
`a263bce-dirty+psxport-d3d67848`. Frame 300 contains 642043/691200 non-black pixels and is
pixel-identical (`AE=0`) to the retained PSX reference despite its older framework provenance. The
reference is not stale. Together with the Authored-mode falsifier, this isolates the enlarged/cropped
EUROCOM layer to the native producer's projection-input or coordinate interpretation, not global OT
ordering or frame timing. No projection constant was changed. The next grounded falsifier is an
input-side comparison between the source-composed projection state and the guest GTE registers at the
same draw; GTE results remain prohibited as native product input.

### Note (2026-08-28, projection-input owner localized)

`model_transform_input_diagnostic` compares the source-captured rotation, translation, OFX, OFY, and
H with GTE control inputs immediately after each retained submitter super. It only records attribution
evidence and never supplies product rendering state. Its focused test proves the matching answer plus
independently forced rotation, translation, and projection mismatches. Exact native PID 3579702 then
completed 301 reconciled frames and returned from native crt0 without a guest-VSync violation, fatal,
or watchdog. Frame 300 retained the enlarged/cropped picture at 266233/691200 non-black pixels. The
denominator was 673 standard comparisons and zero alternate comparisons: all 673 rotations and
translations agreed, while all 673 projection comparisons disagreed first at H (`expected=0x0200`,
`actual=0x0140`). The 1.6 ratio is the observed picture enlargement.

Ghidra decompilation resolves the cause rather than endorsing a scale patch. `0x8009440C` loads retail
H from the camera record at `camera + 0x18` before calling `0x80019F1C`; `0x80019F1C` uses the separate
`projectionGlobals + 4` halfword only in the OFX formula. The title capture had conflated that
horizontal projection scale with GTE H. Standard and alternate captures now retain the horizontal
scale solely for OFX and source H from the camera record. The exact-pin Clang shipping target builds;
transform/comparator/policy tests pass 3/3. Exact post-fix PID 3589261 completed 301 reconciled frames
and returned cleanly. All 673 standard comparisons now match rotation, translation, and projection;
all mismatch counts are zero. Frame 300 rises from 266233 to 340553 non-black pixels, and visual
inspection against the retained before/PSX images shows the 1.6x enlargement and crop are removed:
the full EUROCOM word and subtitle fit at the retail scale. This resolves the projection-input owner,
not 4:3 parity. Large black native polygons still mask much of the purple starfield, and the first/last
letter edges remain occluded versus the PSX reference. The next grounded boundary is to attribute those
black faces to their source model/material records and verify color, texture, and semitransparency
decode; do not compensate with ordering, clipping, or projection arithmetic.

### Note (2026-08-28, damaged-pixel attribution falsifiers)

The exact PSX-path center-writer probe at display coordinate `(56,115)` names opaque untextured
Gouraud packet node `0x800C8D84` as the red letter triangle: guest vertices `(51,116)`, `(51,111)`,
and `(77,116)` with RGB words `0x004218F7`, `0x005A29F7`, and `0x002931FF`. Its bounded payload is
`E1000000/00000000/304218F7/00740033/005A29F7/006F0033/002931FF/0074004D`. This proves the PSX
reference pixel is supplied by authored model geometry rather than texture/CLUT sampling or blending.

Two candidate interpretations were falsified before any product fix was retained. First, changing
the fixed-frame vertex cursor from `(count + 2) * 8` to `count * 8` made the native frame almost
entirely giant purple/black triangles and hid the EUROCOM logo. Generated `0x800193A8` shows why: its
outer topology-group loop re-primes two vertices, then consumes one additional vertex per face, so
each group owns `count + 2` records. The change was removed and a focused two-group regression now
proves the independent restart. Second, the native `(35,115)` census and PSX `(56,115)` packet cannot
be compared: both product paths program a 512-wide display (`GP1(08)=2`), and `PSXPORT_PRIMAT` uses
display coordinates. Output `x=105` therefore corresponds to `x=56` on both paths; the native
`x=35` sample refers to a different output pixel.

The corrected rollback-restored native witness used exact PID `3821484` and exited zero after 301
reconciled frames. Its frame-300 capture returned to the pre-falsifier SHA-256
`45f364ac22ee8224d036a80dd4a6d7463a277d4347c4e21a47b5de528da280b1` and 340,553/691,200
non-black pixels. Visual inspection again shows the whole EUROCOM logo plus the unresolved giant
black/purple occluders. Output `(105,345)` remains `(33,0,66)`. At display `(56,115)`, the native
shipping queue chooses object `0x800A0C74`, sequence 8, key 3312, while source OT chooses object
`0x801E18B0`, sequence 884, key 3160. The title face census is debug-level and its category was not
enabled in that witness, so it did not emit source face/material rows; this run does not establish
the exact face identity. The next diagnostic must preserve this same coordinate and explicitly arm
that existing category; no ordering, filter, cursor, or product-render change follows from the
incomplete identity.

### Note (2026-08-28, exact packet identity and cross-object order owner)

The retained-super packet identity diagnostic closes the missing source identity without changing
product behavior. At frame 299 it reports `2 targets / 13 packet blocks / 26 comparisons / 0
matches`, proving the zero-match path. At frame 300 it reports `2 / 10 / 20 / 2`. Packet
`0x800C2FF4` is allocation block `0x800C2ACC` plus source face 33 (`33 * 0x28`), object
`0x800A0C74`, frame `0x200B`, material `0x01D6`, opaque untextured dark Gouraud. Packet
`0x800C8D84` is allocation block `0x800C85B4` plus source face 50, object `0x801E18B0`, frame
`0x2001`, material `0x003D`, opaque untextured red Gouraud. Both use standard submitter
`0x80019F1C` and the same model asset/data. Generated `0x80019094 -> 0x80019D84 -> 0x800193A8`
proves the allocation block and 0x28-byte per-face cursor.

The damaged pixel is therefore neither missing geometry nor a color, texture, CLUT, transparency,
or blend decode defect. Retail places the red face in OT bucket 3160 and the dark face in bucket
3312, so far-to-near OT traversal paints red last; the buckets differ, so same-bucket AddPrim LIFO
does not decide this pair. Shipping Vulkan `GREATER_OR_EQUAL` instead maps the interpolated native
depths to dark `0.088171428` and red `0.083298709`, selecting dark. Both carried
`authored_depth=0` because the Depth-mode contradiction search groups by object and never compares
this cross-object pair. Mapping their exact authored keys yields dark `0.081545500` and red
`0.082564623`, reproducing the retail winner through the existing frame-wide Authored resolver.

The earlier global-Authored run occurred before the H-source correction and only falsified ordering
as the cause of the then-1.6x scale/crop. It did not test this now-isolated post-projection occlusion.
The implementation therefore seeds Crash Bash's renderer factory with Authored ordering through
`RenderCapabilities`; `Mods::init` applies that default before settings load, preserving an explicit
persisted Depth choice. Generic titles retain Depth. Focused tests cover those three precedence
answers plus the exact cross-object frame-300 pair. A post-change product visual is still required
before 4:3 parity or the occlusion itself can be marked verified.

### Note (2026-08-28, post-change Authored product visual)

Exact native PID `3898998` ran the real USA product for 301/301 reconciled frames and exited zero at
build `87a4e75-dirty+psxport-36e8daa9`. No `psxport_settings.ini` was present, so the Crash Bash
Authored factory default was not masked by a persisted player override. At frame 299 and display
`(56,115)`, the shipping winner and source-OT winner now agree on red node `0x801E18B0`, sequence
884, key 3160; the dark node `0x800A0C74`, sequence 8, key 3312 is submitted earlier. The presented
output correspondent `(105,345)` is RGB `(247,41,74)`, versus the retained PSX reference
`(255,41,82)` and the pre-fix native `(33,0,66)`.

Full-frame inspection verifies that the formerly cut-off `EU`/left letter edge is restored, the
complete EUROCOM word is readable, and no replacement occluder appears. The 960x720 capture remains
340553/691200 non-black, and large black angular regions still mask or replace broad portions of the
purple starfield compared with the PSX reference. The cross-object letter-occlusion defect is thus
verified fixed, while issue 0011 remains investigating because native 4:3 picture parity has a
separate background coverage gap. The next change must identify that gap's source-owned submitter,
face family, or rejection/decode semantic; this result does not justify a depth bias, face skip, or
global ordering change.
