# Project state

| ID | Capability / observable outcome | State | Dependencies | Goals |
|---|---|---|---|---|
| S001 | The selected USA disc, executable, and measured CRASHBSH.DAT code modules are reproducibly identified and derived | verified | — | G001 |
| S002 | The retail boot reaches a guest-visible first frame with faithful drive timing | partial | S001 | G001 |
| S003 | Resident code and every measured CRASHBSH.DAT module form a reproducible executable recompilation substrate | verified | S001 | G001 |
| S004 | Crash Bash graphics are produced natively from decoded game state and look correct | partial | S002, S003 | G001, G002, G003 |
| S005 | The native camera supports wider aspect ratios without changing vertical framing | partial | S004 | G002 |
| S006 | Native camera and world transforms render between simulation ticks | partial | S004 | G003 |
| S007 | Deterministic checks compare the port with independent retail behavior at proven boundaries | partial | S001 | G001, G002, G003 |
| S008 | The retail game modes are reachable and playable end to end on the shipping native path | missing | S002, S004 | G001 |

## The evidence bar

USER 2026-08-30: "Change the directive, pixel matching doesn't matter. I just want working game that
looks correct."

No state item is held `partial` by a residual pixel count, and no item becomes `verified` by a diff
number. An item is verified when the running product does the thing — the mode is reachable and
playable, the scene looks right, the enhancement holds up while playing. The oracle, the differential
harness, and the frame comparators stay as DIAGNOSTICS for finding the cause of a visible defect. The
frame-300 comparison recorded below is retained as history and as a working instrument, not as a gate.

## Current focus

S008 is the current focus: reach and play the retail game modes on the shipping native path and fix
what visibly breaks. S004 continues underneath it, judged on how the game looks while playing rather
than on a per-frame difference count.

S004's prior focus statement follows, for the flow and producer facts it records. Controlled flow is closed through the first playable boundary: the tracked
3,740-frame `replays/flow/crashball-control.pad` crosses the active menu, selects portal 0, advances
the objective and controls pages, starts a Crashball match, then holds Left for 60 frames and Right
for 120. The PSX diagnostic path shows the player ship move from the lower-left to the lower-right;
the exact shipping-native replay completes all 3,740 frames with no recomp miss, fatal, watchdog, or
guest-VSync violation. Issue 0009 is resolved. The boot-flow capability remains partial only on
independent timing and differential evidence, not on product reachability.

The native path renders the Crashball arena, ships, and balls. Its first 2D producer now owns
`0x8002992C` from authored descriptor, position, OT-bin, and color inputs and restores the objective
instructions and top score digits without consuming guest GPU output. The shared flat-color boundary
at `0x80029D28` additionally restores all four top portraits and the two lower markers. The
authored-screen branch of `0x8001A0D8` restores the dimmer and yellow frame; it now retains its source
OT bin in the keyed world ordering domain while textured sprites remain HUD. The indexed
`0x1000`/`0x4000` model-frame recipe restores the two side characters and all four objective arrows
from source animation-bank indices. Exact frame-2500 provenance proves the key-192 semi dimmer stays
behind the key-128 side marker, restoring its retail-bright yellow result. Issue 0023 is resolved.
`0x80018B08` is viewport/camera/render-list setup rather than a drawable. S004 remains partial for
broader native 4:3 parity, not for a known missing Crashball briefing layer.

Frame-300 packet identity now falsifies issue 0015's former DPCS hypothesis: the subtitle face's raw,
modeled, and packet colors and its SXY agree exactly. The residual combined psxport's Vulkan affine-UV
truncation, direct-to-5-bit untextured Gouraud quantization, untextured half-pixel coverage phase,
rounded texture modulation, and an unshifted textured coverage phase. Pins `d6c51535`, `9d370b06`,
`3b033259`, `db30e329`, and `0e6c7e5d` correct those five shared boundaries, reducing the exact frame-300
diff>8 from 98,280 to 6 of 691,200 pixels — one source pixel — while frame 299 is exact. S004 remains
partial for that last pixel and for gameplay-wide parity beyond this frame.

Crash Bash records psxport `0e6c7e5d`, whose generated-substrate identity closes the stale-binary
evidence gap from issue 0018. The current generated identity is
`recomp-2026-08-30.3-bd29ec0f103d99a1e5557f6c20a351704b530a1cf9ff46c2c7495cda59465aa2`.

## Recent evidence

2026-08-30 (first look at the running game): the port was driven to a live Crashball match on the
shipping native path with `replays/flow/crashball-control.pad` and captured at 4:3, 16:9, and with
`PSXPORT_FPS60=1`. Shots are in `scratch/screenshots/report/`.

The 4:3 gameplay picture is correct: arena, floor, all four player ships with their characters, the
puck, the four portraits with their score digits, and the player ship. Nothing is missing or
misplaced.

Widescreen WORKS and S005 remains `partial` while other modes need coverage. At `aspect=1`, the
Crashball arena is a real wider camera view with balanced left/right framing. The objective briefing
is deliberately different: its title-owned dimmer is an authored 4:3 presentation signal, so the
whole composition is clipped to one centred 4:3 viewport at full height, matching the oracle rather
than exposing widened arena columns behind its corners (issue 0024 resolved).

Interpolation makes S006 `partial`. `PSXPORT_FPS60=1` now rebuilds midpoint model geometry from
immutable title snapshots; the clean 3,741-frame replay reports 6,159,397 interpolated primitives
over 3,701 extra presents. Direct midpoint captures show motion between neighbouring real frames,
not duplicate 30 Hz presents (issue 0025 resolved).

2026-08-30 (textured coverage phase): the 45-pixel residual resolves to seven source pixels. Source
`(206,82)` is covered by object `0x801D0AC8`, frame `0x2019`, faces 0 and 3 — textured triangles two
pixels wide at `(206,83)/(206,81)/(207,82)`; source `(320,63)` is covered by a semitransparent textured
face of frame `0x200C`, material `0xA092`, over an untextured base whose retail writer `0x800BEF5C` and
native producer agree exactly. Every differing pixel sits on the edge of a tiny textured primitive while
the model layer underneath matches retail exactly. The cause was that only `tri.vert` carried the PSX
coverage rule: `tritex.vert` left textured coverage on Vulkan pixel centers, which moves a two-pixel-wide
primitive by one pixel. Framework `0e6c7e5d` applies the same half-native-pixel shift to the textured
pipeline and rewinds affine UV from the native pixel's center rather than its corner, and adds
`gpu_vk_texture_coverage_selftest.cpp`, which reports the seeded background `1882` at the edge probe with
`34D4` at its interior control before the fix and `34D4` for both after. This supersedes the earlier
rejection of the textured shift, which kept the corner-relative rewind and was measured before the
modulation fix. The exact 303-frame run reconciles every frame with no recompilation miss or fatal trap,
and presented frames 299/300/301 fall from 99/45/24 to 0/6/6 differing pixels of 691,200. The last
frame-300 difference is source pixel `(118,209)`: native `(11,0,15)` versus retail `(11,0,16)`.

2026-08-30 (PSX texture modulation truncation): source display pixel `(238,61)` binds to retail packet
`0x800C7454`, a dark textured face over an underlying G3 face with exact native/packet geometry and
colors. Retail samples texel `0x8421`, truncates its modulation to zero, and leaves background `0x0C01`
unchanged; native produced `0x1022`. Both Vulkan textured fragment paths rounded the modulated 5-bit
texel instead of computing the PSX's truncating `(texel5 * color8) / 128`, exactly as the software
rasterizer does. Framework `db30e329` corrects both paths and owns the discriminator in its own
`gpu_vk_modulation_selftest.cpp`: one dark texel staged as ABR1 additive, ABR0 average, and opaque, each
destination seeded so a case that never rasterized cannot read as a pass. It passes 3/3 on the shipping
path, and reverting only the shaders reports `1022 / 0801 / 0421` against expected `0C01 / 0400 / 0000`,
whose additive cell is the exact witness pair. The exact 301-frame run at `b3eeaac+psxport-3b033259-dirty`
reconciled 301/301 frames with no recompilation miss, fatal trap, or guest-VSync violation, and its
frame-300 diff>8 against the retained PSX reference falls from 5,546 to 45 of 691,200 pixels (upper 39,
subtitle 0, lower 6). The two remaining clusters are display `(386..387, 252..254)` — native `(16,33,58)`
versus retail `(8,0,33)` — and `(390..391, 249..251)` — native `(16,25,49)` versus retail `(16,8,41)`.
Issue 0015 remains open only for those clusters.

2026-08-30 (PSX affine-UV rounding): exact packet identity maps subtitle packet `0x800C84D4` to
object `0x801E1DB8`, frame `0x2008`, face 9, material `0xA1B2`. Packet/native SXY match exactly;
raw colors are `0x00C6C6C6`, while modeled and packet colors are exactly `0x006C6C6C`. A software-GPU
write chain at absolute VRAM `(61,401)` proves the expected packet and blend are the final writer.
Vulkan sampled the adjacent gray palette texel because it truncated fractional affine UV; the PSX
software path rounds to nearest. Framework `d6c51535` corrects the shared shader and makes its shipping
GPU selftest reachable: UV phase passes 28/28 at 1x/3x across opaque/semitransparent integer and
fractional slopes, while blend equations remain 16/16. The exact frame-300 diff>8 falls 98,280 ->
90,906; subtitle-band differences fall 21,512 -> 14,270. A second exact witness binds untextured G3
packet `0x800C397C` to object `0x800A0C74`/face94 with exact packet/native SXY and colors and DTD off.
Framework `9d370b06` corrects direct-to-5-bit Vulkan quantization; source `(111,25)` becomes exact and
the frame diff>8 falls again to 25,383. A third witness binds packet `0x800C89EC` to object
`0x801E18B0`/frame `0x2001`/face27/material002C with exact SXY and colors. PSX integer-coordinate
coverage includes vertex `(85,137)`, while unshifted Vulkan half-pixel sampling misses it. Framework
`3b033259` aligns only the untextured path and its shipping edge probe changes from `1C04` to expected
`350B`; the exact frame diff>8 falls to 5,546 (upper 466, subtitle 2,829, lower 2,251). The same shift
on the textured path regressed to 15,053 and was rejected. Issue 0015 remains open only for the small
raster residual, not a cue-model discrepancy.

2026-08-30 (Crashball authored cross-layer order): matched frame-2500 evidence identifies the left
objective marker as object `0x8009AC04`, face 178, CLUT row 335, with zero depth cue and packet colors
equal to its raw source colors. Presenting the completed model snapshot selects that retail animation
phase, and packet-quantized SXY restores the thin face's coverage. The final mismatch was the
key-192 semi `0x8001A0D8` dimmer painting after the key-128 opaque marker because it had been forced
into `RQ_OVERLAY`. The authored-screen decoder now retains that quad in the same keyed world domain
as models. An exact 2,502-frame replay exits cleanly; pixel provenance reports authored ord
`0.994125366` for the dimmer behind `0.996078491` for the marker, and the presented capture restores
both side arrows to bright yellow like retail. Issue 0023 is resolved.

2026-08-30 (indexed model frames): retail pixel `(30,105)` selects packet `0x8014814C`, written by
`0x800193A8` under model decoder `0x80019A60`. It is face 48 in block `0x801479CC` for object
`0x801CDAB0`, model data `0x800DBF98`, frame family `0x4000`. That family selects a group descriptor,
follows its `+0x0C` relative frame redirect, and expands 16-bit animation-bank indices into six-byte
XYZ records with low-two-bit flags. The native face exactly matches retail SXY
`(20,108)/(30,107)/(36,102)` and sort key 542. A stable 2,480-frame run emits 4,265,746 native faces
under `0x80019F1C`, restores both side characters and center arrows, reconciles every frame, and drops
zero layers. Interpolated indexed frames remain deliberately unsupported pending their source recipe.

2026-08-30 (authored-screen Gouraud overlay): all five objective-frame `0x8001A0D8` calls carry
`0x10000000` and therefore bypass GTE projection. Their copied source geometry is one full briefing
dimmer plus four border strips at depth bias 384/bin 192. Capturing four XY/RGB vertices with title
offset, scale, fade, blend, and bin restores the dimmer and yellow frame. Classifying that family as
authored world rather than HUD preserves the bin-1 portraits and text above it while retaining its
cross-layer OT relationship with models. The 2,480-frame run emits 5,830 native quads over 1,395
frames and reconciles every frame with zero dropped layers; GTE-mode calls remain guest-only pending
a separate source-transform recipe.

2026-08-30 (flat textured sprites): Ghidra proves `0x80029D28` is the flat-color twin of the first
native quad leaf, with the same descriptor, position, bin, fade, blend, UV/CLUT/tpage, return, and
render-list semantics. Its three static call sites are in `0x800243A0` and `0x80019A60`. A retained
super now feeds the shared decoder with one repeated color and explicit flat shading. Stable frame
2480 restores four portraits and two lower markers without disturbing objective text; the 2,480-frame
run reconciles every frame and emits 3,091 native flat quads across 1,329 frames.

2026-08-30 (first native 2D producer): the retained-super `0x8002992C` capture decodes only authored
descriptor, position, bin, color, display-scale, and fade inputs into the current immutable scene
snapshot. The producer selects the title render-list identity but reads no OT contents, packet, GP0,
VRAM, or framebuffer output. The exact 2,500-frame Crashball replay reconciles 2,500/2,500 frames with
zero dropped layers and records 51,318 native quads across 1,283 frames; visual inspection confirms
the objective instructions and top score digits are restored.

2026-08-30 (packet attribution): Ghidra proves Crash Bash's two parity packet pools are runtime heap
allocations whose base/end/current globals are adjacent to the two static 4,096-entry OTs. psxport's
live descriptor path replaces the structurally blind fixed-window assumption. On the exact objective
frame, attribution changes from zero spans/every packet owner zero to 1,828 spans and 108 direct-pool
rows with owners: `0x8002992C` 92, `0x80029D28` 6, `0x80018B08` 5, `0x8001A0D8` 5. The dominant owner
is a pre-packet textured-Gouraud-quad boundary, not an OT/GP0 reconstruction point.

2026-08-30 (later): deterministic real-pad input drives the title screen through DAT28136 and portal
0 into DAT28241. The portal-selection byte at `0x8005A677` changes from `0xFF` to `0x00`; the following
Cross edge reads 31 sectors from LBA 28241 into `0x800B32B4`. PSX-path captures identify the apparent
static arena as an objective page followed by a controls page, each accepting Cross. The tracked
3,740-frame replay advances both, starts a live match, and visibly moves the player ship left then
right. The same exact replay completes on the shipping native path with no recomp miss, fatal,
watchdog, or guest-VSync violation. Its native frame renders the arena, moving balls, and ships but
omits the diagnostic path's 2D briefing/HUD/character layers (issue 0023).

2026-08-30: the Cross-controlled path added the fourth alternative in the nested slot. DAT28136 is
the exact `CRASHBSH.DAT+0x0367E000` 42-sector image (SHA-256 `c5052413...e43ad`) loaded at
`0x800B32B4`; provisioning now verifies 32/32 facts across BOOT and four nested modules. Regeneration
derives 1,411 roots into 2,593 functions, including 143 DAT28136 functions, without seeding the old
`0x800B4E1C` miss. The strengthened product differential observes callback installation and first
update, and its 2400-frame extension exits 0 with no miss, fatal, or guest-VSync violation.

2026-08-29 (later): the nested 0x800B32B4 slot is a FAMILY of attract modules, not a two-entry
slot. A third member — `31 sector(s) from LBA 28241` — was byte-verified against the miss RAM dump
and provisioned as stem `DAT28241`; current provisioning is 32/32 across five total modules. With it, the
flow runs 9000- and 40000-frame probes to exit 0 with zero recomp-MISS, and the attract demo
scenes render real native content (measured screenshots: lightning temple, character close-up,
complete bumper arena). The menu transition is still the next flow boundary; a START press does
not yet change the app mode. Issue 0018 is resolved: its supposed flaky compiled-case miss combined
an older binary without `0x80012840` and a later regenerated substrate that contained it. The runtime
now announces the exact compiled substrate identity, and the shipping boot verifier requires that
exact current identity before accepting runtime evidence, so this evidence class cannot recur.

2026-08-29: the attract flow now runs 2400/2400 frames to exit 0 with zero keyord fatals, zero
recomp-MISS and zero guest VSync timeout — past the previous f1023 ceiling. Three things landed
together (issue 0017).

The `0x800C3434` boundary was never a discovery gap: `crashbash-cd` names the load outright,
`38 sector(s) from LBA 28272 to 0x800B32B4` — a SECOND module in the same nested slot, read over
MENU once the flow leaves the menu. It is provisioned as overlay stem `DAT28272` (6/6 facts;
current provisioning is 32/32 across five total modules) and emits beside MENU with a distinct signature,
which is what lets the router tell two same-base modules apart. It is named by disc LBA, not role:
its dispatched code registers a behavior vtable at `0x8005AA70` and its name table is animation
states, which rules out "second menu phase" without establishing what it is. `loaded_module.MODULES`
is now the one registry provisioning and the recompiler both read.

Two ordering defects were rooted out, both latent until this module put unlike draws in one frame.
The authored key->ord carrier normalized a FRAME-WIDE OT key by a PER-DRAW `depthLimit`, so the same
key landed in different bands per object; `fixedModelSortKeyOrd` now takes the key alone and the
parameter is gone, making the defect a compile error. Then an OT bucket of 472 tied faces exceeded
what one band can represent in D32 — because depth resolution was the wrong currency. Retail paints
a bucket LIFO, so which face wins inside a bucket is a DRAW-ORDER question: the bucket now shares one
depth and is drawn in reverse through a new `RqItem::draw_seq` kept distinct from submission `seq`.
Bucket size is unbounded and the tie-budget refusal is gone.

Evidence: the tie path fires on real data at up to 1942 faces in one frame; the frame-300 present is
BYTE-IDENTICAL to the retained `native-final-300.ppm`, so the verified EUROCOM result is untouched;
frame 1000 differs from frame 300 by 98.97% of pixels. psxport 121/121, Crash Bash 23/23.

S004 remains partial (issue 0015's frame-300 gradient/raster residual is open). Guest state 0x8004E0B8 and app mode
0x80078C90 are unchanged, so the menu transition is still the next flow boundary.



S004 remains partial. The source-owned producer now copies the title-composed affine/projection
state, decodes direct `0x2000` and two-level `0x5000` fixed-frame records, and reconstructs
`0x800193A8` rejection, winding, and OT bucket keys without using GTE, OT, or GP0 output as product
input. The corrected camera H owner removes the former 1.6x scale/crop; exact PID 3589261 completed
301 reconciled frames with 673/673 source/GTE input matches and visibly restored the full EUROCOM
word and subtitle at retail scale.

The remaining damaged letter pixel is now source-identified. Frame-300 packet identity maps dark
node `0x800C2FF4` to object `0x800A0C74`, source face 33, material `0x01D6`, key 3312, and red node
`0x800C8D84` to object `0x801E18B0`, source face 50, material `0x003D`, key 3160. Both are accepted,
opaque, untextured Gouraud faces. Retail OT traversal makes the lower-key red face the final winner;
native Vulkan `GREATER_OR_EQUAL` instead selects dark D32 `0.088171428` over red `0.083298709`.
Depth mode leaves both `authored_depth=0` because its contradiction search is object-local and the
pair crosses objects. The existing frame-wide Authored resolver maps their keys to red
`0.082564623` over dark `0.081545500`, reproducing retail. Crash Bash now seeds Authored as its
renderer factory default before settings load; generic titles remain Depth and an explicit persisted
player choice wins. Focused Clang tests prove those precedence rules and the exact cross-object pair.
Exact post-change product PID 3898998 then completed 301/301 reconciled frames and exited zero at
build `87a4e75-dirty+psxport-36e8daa9`. With no persisted settings file, the Crash Bash Authored
factory default made shipping and source OT agree on red node `0x801E18B0` at frame-299 display
`(56,115)`. The presented correspondent changed from dark `(33,0,66)` to `(247,41,74)`, versus PSX
`(255,41,82)`, and visual inspection confirms the formerly cut-off `EU` edge and full EUROCOM word
are restored without a new occluder. This verifies the named cross-object letter-occlusion fix, not
native 4:3 parity: the separate large black angular starfield/background holes remain visibly present.
S004 therefore remains partial, and widescreen and interpolation remain downstream of resolving that
coverage gap.

That coverage gap is now source-identified and product-corrected. Retail fixed-model submitter
`0x800193A8` classifies triangles with GTE AVSZ3/OTZ; native incorrectly used the third transformed
SZ, and the title had never published its post-initialization ZSF3. The retained-super owner at
`0x80033494` now publishes actual post-write CR29/CR30 and the classifier applies signed, saturated
AVSZ3. Exact PID 4054917 completed 301/301 reconciled frames at
`138ad0a-dirty+psxport-ff21584d`: object `0x800A0C74` face 261 has matching packet/native SXY,
ZSF3 341, OTZ 1511, is accepted at sort 1755, and writes display `(234,181)`. The formerly black
output `(439,543)` becomes `(66,8,90)` versus PSX `(66,0,90)`. Full-frame inspection verifies the
large angular holes close without regressing the EUROCOM word or subtitle. S004 remains partial only
because finer facet/gradient and logo-color differences still prevent whole-frame 4:3 parity.

## Capability details

### S001 — Reproducible retail inputs

Evidence: `tools/provision.py`, `tools/loaded_module.py`, and `tests/test_provision.py` verify the USA
`SCUS_945.70` identity plus the full DAT, BOOT, and MENU payload measurements recorded under
`titles/crashbash/`. Provisioning passes 11/11 controlled positive/refusal/mutation checks; no game
bytes are tracked.

### S002 — Faithful boot to first frame

The port preserves the independent Beetle interrupt-exit prefix, executes both measured loaded
modules, and the landed CDC phase candidate crosses the former GetTN boundary through later reads.
On former exact pin `99a42aa3`, the product completed both loads and printed `empty prims`,
but an intentional bootstrap black clear incorrectly marked the main presenter ready and the process
exited 134 on the steady `[watchdog] STUCK` timeout. Exact recorded pin `784e5212` fixes that phase
ownership with typed main-frame versus transition completion. Its serialized trace observes the
deterministic order `2/2 loads -> empty prims -> MENU 0x800B5244 from ra=0x8001E7C0` without STUCK,
fatal output, or a recompilation miss, but it is not a passing product gate: seven guest
`VSync: timeout` lines were previously accepted. `tools/verify_boot.py` now rejects them and passes
13/13 controlled answers. The current native-ownership candidate has a typed fatal VSync trap,
finite boot/process ownership, and retained display/GPU overrides. Its first serialized launch
proved the trap and named boot-time memory-card setup `0x800486DC` as the next owner before MENU. A
second serialized launch validated that owner, then identified Sony libcd's controller/IRQ timeout
path through `0x8003E6B0` and `0x8003EBF8`. The current Clang build retains both generated bodies,
owns the controller handshake at `0x80034B8C`, and uses the framework's synchronous CD command, sync,
and ISO lookup owners. A third serialized launch had zero VSync violations/timeouts and exposed the
distinct `GetTN` status hang at `0x800349AC -> 0x8003584C`. The rebuilt candidate owns that top-down
readiness contract from the actual CHD TOC. A fourth run validated it and reached the distinct guest
async file-read path at `0x80027790 -> 0x8003470C`. The rebuilt candidate performs that read
synchronously from the real CHD, with the generated body retained. A fifth serialized launch passes
the strict `2/2 loads -> empty prims -> MENU` gate without a VSync violation. The direct 120-frame
run then names disc/license state machine `0x8002D4F4` as the next fatal root and captures two fully
black presents. A first synchronous owner rendered only the red copy-protection failure screen,
falsifying its state-16 interpretation. The corrected owner follows measured state-18-to-0 success;
its direct launch accepted the authentic disc and removed that failure path, then exposed an
independent missing HwCARD completion after an empty firstfile result. A focused shared psxport
regression now proves firstfile and nextfile each preserve zero and invoke the interrupt-mode
completion callback exactly once; another serialized launch remains required.

The memory-card boundary is now resolved. Issue 0013 records the root cause: the framework entry that
publishes the card as a BIOS device was never called by this title, and libmcrd's own device-table
walk at `0x8004799C` returns 0 for "no such device" — the same value it returns for "request
started" — so the caller waited forever on an operation that was never begun. A three-way A/B on the
real disc proves the title wiring and the shared completion event are each necessary and neither is
sufficient alone.

The port now reaches the Crash Bash TITLE SCREEN. Two defects were resolved to get there. Issue 0014:
the emitter was binding a direct call into the nested MENU range to the BOOT module's own stale body,
so the port executed the wrong module's code once MENU loaded; direct calls into a narrower
overlapping module's range now route through the resident-overlay router. Issue 0012: the two BOOT
object callbacks `0x8008ADA4` and `0x8008BB48` are natively owned in `boot_object_callbacks.cpp`,
taking guest-VSync ownership to 25 of 51 sites with 26 guarded residuals.

The 600-frame product run now exits 0 with no guest-VSync violation, recompilation miss, watchdog
stall or fatal trap, and the guest submits 737,668 primitives (previously 2). Rendered through the
diagnostic PSX path as an oracle it presents the title screen at 92.9%/98.9%/83.6% non-black, which
proves the simulation is producing a real picture. The SHIPPING native path still presents black
because no native producer exists — that is S004, below.

Pad delivery is now verified independently of flow transition. Crash Bash's registered class-2
interrupt element uses VBlank `I_STAT` bit 0 to enter handler `0x8003B224`, drives SIO0 with
per-byte `I_STAT` bit 7 acknowledgements, and times the exchange with root counter 2. Shared
psxport now owns those hardware boundaries. The frame-200 idle/START A/B recorded in resolved
issue 0019 and `docs/findings/crashbash-pad-sio.md` proves the exact driver, parser, and
game-facing P1 values; no direct buffer injection or title-local input path exists.

Gap: Issues 0009, 0012, 0014, 0019, 0020, 0021, and 0022 are resolved. The verified active menu accepts
Cross, queues table `0x800B8E50`, installs DAT28136 callback `0x800B4694`, and executes it; START is
correctly ignored. Controlled input continues through portal 0, loads DAT28241, advances both briefing
pages, starts a live Crashball match, and moves the player ship in both directions. Independent
full-memory/timing comparison remains under S007; missing native picture layers remain under S004.

### S003 — Executable recompilation substrate

Evidence: the 2026-08-30 shared emitter applies one merge/prune pipeline to return-delimited functions
in both resident MAIN and loaded modules. Verified retail emission derives 2,593 functions: 1,355
resident, 710 BOOT, 89 MENU, 190 DAT28272, 106 DAT28241, and 143 DAT28136. Removing five redundant MAIN seeds leaves
all five addresses dispatchable and preserves the exact output hash; only `0x8003B1BC` remains a
manual MAIN seed because current binary analyses do not derive it. Compared with the old emitter on
the same inputs, the added 391 functions cost 146,697 bytes of C (+0.56%) under the unchanged size
guard. The rebuilt Clang product passes the strict `2/2 loads -> MENU 0x800B5244` boundary with zero
miss, fatal, VSync-timeout, or watchdog output. Generated code remains ignored and reproducibly rebuilt
from verified external inputs. This proves the recompilation substrate, not later gameplay coverage.

### S004 — Native graphics producers

Partial capability: Crash Bash now has one native producer that reads decoded game camera/object/
material state and submits fixed-frame untextured and textured geometry. `tools/inventory_render_anchors.py` binds its answer to the exact generated
input/output hashes and psxport commit. Both previous pin `17981527` (1,724 functions) and substrate
baseline `99a42aa3` (2,005 functions) contain 31 projection anchors and 17 camera-control anchors,
including the stable resident `0x80015780 -> 0x8001CD04 -> {0x800193A8, 0x8001AF2C}` chain, although
three individual anchor addresses differ. `CrashBashRuntime` now declares the intended
`interpolatedNative` product policy, but policy does not create a producer; runtime attribution and
state semantics remain unproven.
`tools/verify_render_anchor_reach.py` passes 8/8 known/unknown/zero/empty/fatal controls and NOW HAS
ITS FIRST REAL POSITIVE TRACE: once the BOOT object callbacks were owned, a 600-frame run produced
8 histograms and 765,553 projection calls attributed to exactly the two anchors the static inventory
predicted, `0x800193A8` and `0x8001AF2C`, reached from `0x80019DAC` and `0x80019D7C`. That proves
runtime projection ancestry only; camera semantics, scene state, primitive ownership, 4:3 parity,
widescreen and interpolation all remain unproven. See issue 0011 and
`docs/findings/render-anchor-inventory.md`.

The clean `0d4712f` product A/B established the original gap directly: 600/600 native-owned frames complete on
both paths with no guest-VSync violation, but the shipping native captures are entirely black while
the PSX diagnostic captures visibly show Eurocom and the Crash Bash title. Semantic decompilation
now traces the live anchors backward through model decoder `0x80019A60` to pre-GTE object submitters
`0x80019F1C` / `0x8001DD50`, transform composer `0x8001965C`, and source-vertex decode/interpolation
`0x8001C1E0` / `0x8001C0F0`. A working candidate captures those accepted object/model/transform
inputs into two title-owned frame snapshots while retaining the generated supers. A 350-frame
real-disc run reaches the standard capture 17,004 times and returns cleanly; the history rotation
selftest passes, and a second run reports 71 accepted model draws in frame 255. The current producer
extends that boundary through source fixed-frame topology/material/UV decode and 4:3 native
submission: frame 255 captures 1,542 faces, of which 382 are textured, submits 1,010, and produces a
visibly non-black 90.81% native picture versus the 96.67% PSX reference. A retained untextured capture
compared with the new capture visibly proves the textured faces contribute sampled gradient squares
and particles. The next keyed-order candidate exposes the complete EUROCOM letter set at frame 300,
but visibly enlarged/cropped and occluding most of the starfield. The copied `0x8001965C` affine and
`0x80019F1C` projection calculation agree with retail, and `native_projection` is GTE-differentially
verified, so a projection scale factor would be a bandaid. Exact PID 3396467 forced global Authored
mode and completed cleanly, but the frame remained enlarged/cropped at 266224/691200 non-black versus
Depth's 266233/691200. This falsifies cross-object OT policy. Current-build PSX-path PID 3410230
completed 301 frames cleanly at build `a263bce-dirty+psxport-d3d67848`; its 642043/691200 frame-300
capture is pixel-identical (`AE=0`) to the retained PSX image. The retained reference is therefore
not stale, and the scale/crop belongs to the native projection-input/coordinate boundary. The
post-run zero-depth branch correction has a controlled unit test but no second native visual claim.

The later source-identity and ordering work is now product-verified for the damaged letter boundary.
Exact PID 3898998 used Crash Bash's Authored factory default against landed psxport `36e8daa9`,
reconciled 301/301 frames, and exited zero. At frame-299 display `(56,115)`, shipping and source OT
both select red source node `0x801E18B0` (sequence 884, key 3160); output `(105,345)` is
`(247,41,74)` versus PSX `(255,41,82)`. The 960x720 capture contains 340553/691200 non-black pixels
and visibly restores the full EUROCOM letter edge with no replacement occluder. The large black
angular holes across the purple starfield remain, so this closes only the cross-object ordering
defect and leaves overall native 4:3 image parity partial.

The subsequent background investigation binds PSX packet `0x800C5394` to native object
`0x800A0C74`, frame `0x200B`, face 261, material `0x02FB`. Source vertices, transform, projection,
material, and packet/native SXY match retail exactly. Exact PID 4054917 proves the retained
`0x80033494` initialization publishes ZSF3 341, yields OTZ 1511/sort 1755, accepts the face, and
writes display `(234,181)`. Full-frame comparison shows the broad angular holes closed with the logo
intact.

The logo-color half of the residual gap is now closed by replaying the retail-submitted colors.
Retail bakes DPCS depth-cueing into the vertex colors it submits; the native producer was ignoring
the captured retail colors and rendering the raw pre-cue palette entries, washing the EUROCOM
rainbow gradient toward its raw hues. `submitFixedModel` now renders `face.retailColors`, the
captured `applyModelDpcs` output, with no double application (the native queue applies no depth cue
of its own). Exact 301-frame product runs at pin `02430b1b` exit 0 with zero recomp misses; the
frame-300 present is 676474/691200 (97.87%) non-black, the EUROCOM rainbow and subtitle visibly
match the PSX reference, and the full-frame difference is 409009/691200 pixels with the strong
differences (>90 sum) confined to sub-pixel offsets on thin white subtitle glyphs and background
facets, not the logo. Native 4:3 parity remains partial: the residual is that facet/gradient and
sub-pixel class, now measurable with the retained `scratch/screenshots/psx-ref-300.ppm` reference.

The same verification session resolved a substrate regression that had silently broken boot at
framework HEAD: the libmcrd interrupt-mode card-event callback `0x8004718C` was invisible to static
discovery (guest-side construction `addiu a3, a3, 0x718C` at `0x80047280`, host-owned event table,
never a RAM word), so every run since the function left the derived set failed fast on the first
boot-frame card probe; recorded as issue 0016. Crash Bash seeds it as a measured live boundary, and psxport
`02430b1b` generically accepts a padded jr-ra boundary as a function entry. The BOOT overlay's
legitimate emission density is measured at 51.7x of its image (20.0 MB of C from 387,072 bytes,
verified at the pre-guard pin), so `tools/recomp_bootstrap.py` sets the size-guard knob to 56 with
that measurement recorded rather than raising the framework default.

Gap: smaller facet/gradient and sub-pixel glyph differences remain against the PSX reference. Native 4:3
parity, widescreen, and interpolation remain unverified.

### S005 — Widescreen camera

Missing capability: no PC-owned Crash Bash camera/projection producer exists. The nearest measurable
gate is runtime proof of the active projection-state owner, then 4:3 native parity, followed by wider
horizontal view with unchanged vertical framing. GTE/OT/GP0 output cannot be the source.

### S006 — Interpolated presentation

Missing capability: the PC-owned history now retains consecutive copied camera/world transform and
fixed-face snapshots, but no presentation-time render consumes an interpolated pair. The nearest
measurable gate is unchanged simulation with interpolation disabled, then an alpha-0.5 native render
between verified snapshots without guest-RAM mutation.

### S007 — Differential verification

The independent Beetle oracle and title-local verifiers establish interrupt ordering, module loads,
command phase progress, and the still-divergent guest completion result. Static render inventory now
has controlled positive and negative fixtures.

Gap: full-memory lockstep, a passing clean pinned CDC/completion run, first-frame runtime submitter
attribution, and native 4:3 image parity are not yet demonstrated. The
old exact-pin watchdog refusal and the corrected exact-pin `784e5212` MENU gate are valid evidence for
their declared boundaries; neither satisfies any of those later gates.
