# RE frontier

Tracked with the shared `re_frontier.py`; this is the ordered dependency chain from the real disc toward
a faithful port. `re-verified` requires executable evidence plus a real verification, never merely a
successful build.

USER 2026-08-30: "Change the directive, pixel matching doesn't matter. I just want working game that
looks correct."

A step's verification is the running product doing the thing, not a pixel difference count. Frame
comparison stays available for locating the cause of a visible defect; it never defines whether a step
is done, and no step is blocked on a residual pixel.





## boot

### boot.target — Select and measure the retail target
- status: re-verified
- deps:
- evidence: USA SYSTEM.CNF boots SCUS_945.70; the executable contains the North America and BASCUS-94570 markers; SHA-256 fd5727a18feb2a2d5a6359a55966f0266284d1e50f64ee9b8a127a97091bd516; PS-X EXE entry 0x8002E7B0, load 0x80010000, text 0x69000; crt0_extract decoded 36 instructions and resolved 8/8 boot fields.
- where: titles/crashbash/executable.json, titles/crashbash/README.md
- gap:
- notes: Identity and CRT0 evidence only. No game seam, recompilation, or boot is claimed.

### boot.provision — Resolve the selected disc and extract the executable reproducibly
- status: re-verified
- deps: boot.target
- evidence: tools/provision.py verifies the real USA CHD: SYSTEM.CNF boot 1/1, executable identity/header 11/11, and 32/32 facts across BOOT plus four measured alternatives in the shared nested slot. The newest member is DAT28136: CRASHBSH.DAT+0x0367E000, 0x15000 bytes, SHA-256 c5052413c19fcab896ffa19d18b278f4418181d6c927bc903f9cc3de9e6e43ad, loaded at 0x800B32B4. tests/test_provision.py exercises positive, mismatch, refusal, precedence, ambiguity, shared-source, bounded-offset, entryless-module, and atomic-publication behavior through the shipping path.
- where: tools/provision.py, tools/loaded_module.py, tests/test_provision.py, titles/crashbash/executable.json, titles/crashbash/*_module.json
- gap:
- notes: The disc, full 73,220,096-byte DAT, executable, and extracted modules remain external/gitignored. The tracked manifests prove identity and provisioning only, not game behavior.

### boot.recompile — Generate the static-recompilation substrate
- status: re-verified
- deps: boot.provision
- evidence: The verified USA image, residual measured targets, retail-derived HookEntryInt setjmp re-entry, and five measured CRASHBSH.DAT modules emit 1,411 roots into 2,593 functions: 1,355 resident, 710 BOOT, 89 MENU, 190 DAT28272, 106 DAT28241, and 143 DAT28136. The former 0x800B4E1C miss is discovered from the whole DAT28136 image and is not a manual seed. The emitter fingerprints the exact emitted translation units and routing metadata as `recomp-2026-08-30.3-bd29ec0f103d99a1e5557f6c20a351704b530a1cf9ff46c2c7495cda59465aa2`; tools/recomp_bootstrap.py binds that stamp to the compiled table, cache, and shipping RecompRegistry adapter and passes 10/10 positive/forced-negative checks. A Clang product build logs the same installed identity.
- where: tools/recomp_bootstrap.py, tools/loaded_module.py, titles/crashbash/*_module.json, game/recomp_seeds.json, game/core/game_config.cpp, game/core/recomp_register.cpp, generated/ (gitignored)
- gap:
- notes: Generated output is rebuilt from the verified executable/modules and remains untouched, gitignored, and non-authoritative. The main_reentry seed is mechanically tied to ResetCallback 0x80031A80: its sole jal to setjmp 0x8003ACEC resumes at call+8, 0x80031AE8.

### boot.harness — Establish a deterministic psxport/oracle boot comparison
- status: re-verified
- deps: boot.recompile
- evidence: The independent Beetle interpreter ran the actual USA CHD for 600 frames and repeatedly showed the saved-context path 0x80031AE8 (v0=1, sp=0x80068B14) -> 0x80031B58 (same sp, ra=0x80031AF8). The current Clang port preserves that prefix and IRQ2 callback 0x8003F5F0 -> drain 0x8003E14C. The strict serialized product now completes both loaded-module requests, prints `empty prims`, then reaches measured MENU entry 0x800B5244 from ra=0x8001E7C0 without a watchdog stall, fatal, recomp miss, or guest-VSync violation. The opcode-backed owner map measures 51 resident-plus-BOOT guest-VSync sites. Successive fatal-trap runs identified memory-card startup `0x800486DC`, libcd command/sync `0x8003EBF8 -> 0x8003E6B0`, TOC readiness `0x800349AC -> 0x8003584C`, the file-read starter `0x80027790 -> 0x8003470C`, and the later disc/license state machine `0x8002D4F4`. The rebuilt working tree owns 21 sites, leaves 30 guarded residuals, performs descriptor-relative file reads synchronously from real CHD sectors, and enters the measured authentic-disc idle state from real layout evidence. That live path removed the red failure screen and exposed a memory-card wait at 0x800476EC. The shared missing-HwCARD-completion fix alone did not clear it; a card/event trace proved firstfile was never reached because this title never called card_overrides_init, leaving the "bu" BIOS device absent from the kernel device table its libmcrd walks itself (issue 0013). With both the title wiring and the shared completion event in place, the direct 120-frame product run exits 0 and completes 120/120 frames with no watchdog stall, fatal trap, recompilation miss, or guest VSync timeout; a three-way A/B proves each fix necessary and neither sufficient alone, and the focused shared firstfile/nextfile callback regression passes. tools/verify_native_ownership.py passes its real retail check and 7/7 controls; tools/verify_oracle_irq.py passes the real comparison and 4/4 selftests; the corrected tools/verify_boot.py passes 13/13 controlled answers and accepts the strict 67-line trace.
- where: tools/verify_oracle_irq.py, tools/verify_boot.py, tools/verify_native_ownership.py, docs/findings/vsync-owner-map.md, game/recomp_seeds.json, game/core/crashbash_runtime.cpp
- gap: No guest-VSync trap is reached through the 2400-frame controlled Cross path. Continue extending independently controlled flow; any newly reachable guest-VSync target must fail fast and move behind the existing native frame owner before that path can be accepted. Graphics parity remains tracked separately under graphics.camera-submitters. Zero guest-VSync timeout/violation lines remain permitted.
- notes: The oracle comparison is deliberately at the custom exception-exit boundary shared by both machines. It does not claim full-RAM lockstep or that later CD command response timing agrees.

### boot.first-divergence — Reach and root-cause the first real divergence
- status: re-verified
- deps: boot.harness
- evidence: The shared BFRD latch preserves Crash Bash's split PVD read: at the 512-word DMA, LBA=16, data_rd=12, data_n=2340, and the next bytes are 01 43 44 30 30 31 01 00 (type 1 + CD001). The independent Beetle interpreter returns success from the 189-sector starter, then exposes requested/remaining 189 at 0x80027790 and the completion-pending result 1 before settling to 0 at expected sector 0x8C94. On psxport 692b9b20 the same request has already completed before the starter returns: starter v0=0, 0x80027790 v0=-1, active/remaining/async=0, and no 0x800348A8 or 0x80027944 event. tools/verify_read_completion.py passes 5/5 synthetic both-answer checks and rejects that live instant-completion trace.
- where: external/psxport/runtime/recomp/cdc_native.cpp, tools/verify_boot.py, tools/verify_read_completion.py, docs/issues/0005-crash-bash-iso-lookup-reads-the-pvd-payload-from.md, docs/issues/0006-crash-bash-continuous-readn-stops-after-the-firs.md, docs/issues/0007-crash-bash-repeats-the-189-sector-crashbsh-dat-r.md
- gap:
- notes: The root is recursive sector/IRQ delivery from BFRD before the guest read-start function can return. The current watchdog stop occurs during active CHD decompression and cannot prove a guest freeze. Disabling it or adding a game-local file-read override would hide the drive-timing defect.

### boot.drive-timing — Reproduce asynchronous drive-sector availability
- status: in-progress
- deps: boot.first-divergence
- evidence: The true oracle's read starter returns 1 with 189 sectors still pending; its initial read/sync result is 189, then completion returns 1 before settling to 0. The deterministic drive schedule first landed in psxport 3418a79b and was directly exercised through d2266f4b: mode-0xA0 sector events use the same 225,792 CPU-tick period, and that live boot completed both measured module loads without retry or depletion before MENU executed. The last strict live trace still diverged at completion: v0=1/rem=0/expected=0x8C94/async=1 existed transiently, but async cleared before either 0x800348A8 or 0x80027944 returned; only sync return 0 was observed. The oracle-derived command phase machine was pushed in psxport 8611d756 after 100/100 combined framework tests; previous pin 17981527 was a descendant with no CDC-source changes. The earlier candidate trace crosses the former GetTN boundary and reaches later continuous reads through LBA 17655; tools/verify_cdc_phase_progress.py accepts all 8,606 lines, proves all 5 Pause INT3/INT2 pairs use distinct IRQ-handler entries, and passes 8/8 controls including a coalesced-edge negative. The old exact-pin 99a42aa3 run exited 134 because bootstrap black presentation prematurely selected the steady watchdog phase, not because CDC progress stopped. Exact pin 784e5212 removes that false alarm and reaches MENU 0x800B5244 without STUCK, but its boot log has no CDC diagnostics and cannot answer this frontier. tools/verify_read_completion.py still correctly refuses the separate guest-visible completion-state divergence while its 5/5 selftest proves the opposite answer.
- where: external/psxport/runtime/recomp/cdc_native.cpp, tools/verify_read_completion.py, docs/issues/0007-crash-bash-repeats-the-189-sector-crashbsh-dat-r.md
- gap: The historical asynchronous hardware path still lacks an exact clean capture of the oracle's guest-visible result 1 -> 0. That is required only if a generic async-CD mode is revived; the shipping product now owns the measured file-read boundary synchronously.
- notes: The shipping owner must continue reading real CHD sectors and must not fake completion, return a guest VSync clock, or relax the watchdog.

### boot.loaded-modules — Provision and execute every measured loaded code module
- status: re-verified
- deps: boot.first-divergence
- evidence: Real CDC/DMA traces identify BOOT at LBA 35799 and four mutually exclusive images at nested base 0x800B32B4: MENU LBA 28178, DAT28272, DAT28241, and Cross-selected DAT28136. Provisioning verifies the full 73,220,096-byte DAT and every payload identity (32/32 facts); emission retains all five bodies. The exact-identity Cross differential reaches MENU 0x800B5244, schedules 0x800B8E50, observes DAT28136 registration 0x800B4E1C replace app callback 0x80093038 with 0x800B4694, and observes that update execute. Its 10/10 judge falsifiers reject missing/wrong transition evidence, and a 2400-frame extension exits 0 with no fatal, recomp miss, or guest-VSync violation.
- where: tools/loaded_module.py, tools/provision.py, tests/test_provision.py, tools/recomp_bootstrap.py, tools/verify_boot.py, tools/verify_menu_accept.py, titles/crashbash/*_module.json, game/recomp_seeds.json, game/diagnostics/menu_boundary.cpp
- gap:
- notes: No DAT or module bytes are tracked. The old IRQ-timed CDC experiment remains oracle evidence, not the shipping ownership model. Shipping CD work is host-synchronous: bind measured libcd chokepoints to real native disc operations, retain generated bodies for comparison, and never return a guest VSync clock or relax the watchdog. Issue 0009 remains open for the process-driving clean completion gate and resulting post-MENU no-present cause.


## flow

### flow.pad-input — Deliver host pad state through the guest's own SIO driver
- status: re-verified
- deps: boot.loaded-modules
- evidence: Retail code registers SysEnqIntRP class 2 element 0x8006D984 with +8 verifier 0x8003B1BC testing VBlank I_STAT bit 0 and +4 handler 0x8003B224 driving SIO0, per-byte I_STAT bit 7, and timer 2. The shared implementation models those exact boundaries. Fresh finite headless frame-200 idle/START RAM A/B proves the shipping chain: packet 0x80077FBC is 41 5A FF FF -> 41 5A F7 FF, parsed P1 0x80063A92 is FFFF -> FFF7, and active-high game P1 0x8005133C is 0 -> 8; P2 0x80051394 remains 0. RAM SHA-256 values and the rejected direct-buffer shortcut are recorded in docs/findings/crashbash-pad-sio.md and resolved issue 0019.
- where: external/psxport/runtime/recomp/io_peripherals.{h,cpp}, external/psxport/runtime/recomp/sio_pad.{h,cpp}, external/psxport/runtime/recomp/timing.{h,cpp}, docs/findings/crashbash-pad-sio.md
- gap:
- notes: Do not feed masks into 0x8007787C or padSlot0Buf; the verified guest SIO path is authoritative.

### flow.menu-accept-transition — Prove the active menu's Cross transition
- status: re-verified
- deps: flow.pad-input
- evidence: Issue 0019 proves input delivery. Static retail analysis resolves issue 0020's wrong START premise: state index 0x28 has type 0x0101, so FUN_8007F314 takes 0x8007F3C4 before its generic START scan. Active process table 0x800B8E28 updates through FUN_800B3CA8, which reads rising-edge 0x80051380 and accepts Cross 0x4000 at 0x800B3D88-0x800B3D8C; selection zero schedules table 0x800B8E50 through pending slot 0x8009F8A8. The exact-identity shipping differential passes: idle/START each execute 88 active MENU updates, three START edges produce zero accepts, and one Cross edge schedules 0x800B8E50 exactly once before the old callback stops after eight updates. The extended Cross leg then proves DAT28136 registration and update execution; its judge passes 10/10 controlled answers.
- where: game/diagnostics/menu_boundary.cpp, tools/verify_menu_accept.py, docs/issues/0020-active-menu-was-incorrectly-expected-to-accept-start.md, docs/issues/0021-cross-menu-acceptance-needs-an-idle-start-cross.md, docs/issues/0022-cross-transition-loads-an-unprovisioned-dat28136.md
- gap:
- notes: Do not add a START mapping. Input delivery is closed and must not be bypassed or reimplemented. The queue observation alone is insufficient: this boundary is accepted only when the successor registration and callback execution are also observed.

### flow.crashball-control — Reach and control a live Crashball match
- status: re-verified
- deps: flow.menu-accept-transition
- evidence: Tracked replays/flow/crashball-control.pad contains 3,740 active-low pad masks from boot. PSX-path captures prove portal 0 leads through objective and controls pages into a live DAT28241 match; frames 3560..3619 hold Left and 3620..3739 hold Right, visibly moving the player ship across the arena. Exact build a3a5fbd+psxport-a0c18b9e consumes all 3,740 masks on the shipping native path and exits with no recomp miss, fatal, watchdog, or guest-VSync violation.
- where: replays/flow/crashball-control.pad, docs/issues/0009-zero-latency-gettn-response-is-drained-before-cr.md, docs/issues/0023-crashball-gameplay-is-controllable-but-the-nativ.md
- gap:
- notes: The PSX path is a diagnostic visibility oracle only. The shipping native frame owns presentation and renders the arena, ships, balls, briefing/HUD, portraits, markers, and objective characters; issue 0023 is resolved.

## graphics

### graphics.crashball-2d-submitters — Own the missing Crashball briefing/HUD sprite family
- status: re-verified
- deps: flow.crashball-control, graphics.camera-submitters
- evidence: Ghidra decompiles 0x800274FC/0x800276C4 as the two heap-pool allocators and identifies their base/end globals. The exact objective frame records 1,828 writer spans and attributes all 108 direct-pool OT rows. Function 0x8002992C owns 92 opcode-0x3C textured Gouraud quads; its retained-super capture decodes the authored descriptor, packed position, OT bin, four colors, display scale, and fade before packet construction. Its flat-color twin 0x80029D28 owns six opcode-0x2C quads and differs only by repeating one authored color. All five objective-frame 0x8001A0D8 calls take its authored-screen branch: one copied XY/RGB quad is the dimmer and four are the border, at depth bias 384/bin 192. The combined native path restores text, scores, portraits, markers, dimmer, and yellow frame without reading packet/OT/GP0/VRAM/framebuffer output. In the cached/model population, pixel `(30,105)` selects packet `0x8014814C`, written by `0x800193A8` under `0x80019A60`; it is face 48 of block `0x801479CC`, object `0x801CDAB0`, frame family `0x4000`. The source resolver follows the descriptor `+0x0C` redirect and expands animation-bank vertex indices. Native face 48 exactly matches retail SXY `(20,108)/(30,107)/(36,102)` and sort key 542. The 2,480-frame run emits 4,265,746 native model faces, restores both side characters and center arrows, reconciles every frame, and drops zero layers.
- where: game/render/sprite_quad_capture.cpp, game/render/sprite_quad_decode.cpp, game/render/native_sprite_quad_producer.cpp, docs/findings/crashbash-packet-pools.md, docs/issues/0023-crashball-gameplay-is-controllable-but-the-nativ.md
- gap:
- notes: Exact frame-2500 provenance proves the authored-screen key-192 semitransparent dimmer stays behind the key-128 model marker, restoring both side markers to retail-bright yellow. Indexed-frame interpolation remains refused until its source recipe is proven and belongs to `graphics.enhancements`. `0x80018B08` is viewport/camera/render-list setup, not a drawable boundary. The active heap bases are not constants; the config names their descriptor globals and the framework keeps the two live ranges separate.

### graphics.camera-submitters — Identify native camera state and graphics submitters
- status: in-progress
- deps: boot.drive-timing, boot.loaded-modules, graphics.render-anchors
- evidence: The clean 0d4712f Clang product completes 600 native-owned frames with no guest-VSync violation on both paths. Native captures were initially 0/691200 non-black while visually inspected PSX diagnostic captures showed Eurocom and the Crash Bash title. Live rtpcaller evidence attributes 765,553 projection calls to 0x800193A8/0x8001AF2C. Ghidra traces those backward through model decoder 0x80019A60 to object submitters 0x80019F1C/0x8001DD50, transform composition 0x8001965C, source-vertex decode/interpolation 0x8001C1E0/0x8001C0F0, and packet-source prefill 0x80017EE8. The producer retains every generated super and decodes fixed-frame source vertex/topology/material/texture state. Retail 0x800193A8 further grounds third-SZ depth rejection, winding and OT bucket semantics; 0x800159C4 -> 0x8001DD20 grounds the two-level 0x5000 frame source; 0x8001D894 grounds the alternate transform. A Clang build at recorded psxport fb08d30f passes 16/16 CTests. Serialized native PID 3368774 completed 301 frames cleanly and its frame-300 keyed output exposes the complete EUROCOM letter geometry rather than isolated central fragments, but the layer appears enlarged/cropped and occludes most of the starfield. Exact PID 3396467 then forced global Authored mode; it completed cleanly but retained the same scale/crop at 266224/691200 non-black versus Depth's 266233/691200, falsifying cross-object OT policy. Current-build PSX-path PID 3410230 completed 301 frames cleanly at `a263bce-dirty+psxport-d3d67848`; its 642043/691200 frame-300 capture is pixel-identical (`AE=0`) to the retained PSX reference. This falsifies stale timing/provenance and isolates the defect to the native projection-input/coordinate boundary. A post-run branch audit corrected SZ3==0 rejection to the untextured path. Exact PID 3579702 then completed 301 frames with 673/673 source/GTE rotation and translation matches but 673/673 H mismatches (`0x0200` captured, `0x0140` installed). Ghidra `0x8009440C` proves H comes from `camera + 0x18`; `0x80019F1C` uses the separate `projectionGlobals + 4` value only for OFX. Exact post-fix PID 3589261 completed 301 frames with 673/673 full input matches and zero mismatch categories. Its 340553/691200 frame-300 capture visibly removes the 1.6x scale/crop, while black native faces still occlude starfield and glyph edges. Exact post-order PID 3898998 then completed 301/301 reconciled frames and exited zero at `87a4e75-dirty+psxport-36e8daa9`. With the Authored title default active, frame-299 display `(56,115)` makes shipping and source OT agree on red node `0x801E18B0`; output `(105,345)` is `(247,41,74)` versus PSX `(255,41,82)`. Visual inspection verifies the full EUROCOM letter edge is restored with no new occluder, while the separate black angular starfield/background holes remain.
- where: game/render/
- gap: Overall native 4:3 parity remains partial. Exact packet identity falsifies the former DPCS-cue hypothesis. psxport `d6c51535` corrects affine-UV truncation; `9d370b06` corrects ordinary untextured G3 quantization and DTD state; `3b033259` aligns PSX integer-coordinate untextured coverage with Vulkan pixel centers; `db30e329` truncates texture*color modulation as the PSX and the software rasterizer do; `0e6c7e5d` carries the same PSX integer-coordinate coverage rule through the textured pipeline and rewinds affine UV against it. Exact frame-300 diff>8 falls 98,280 -> 90,906 -> 25,383 -> 5,546 -> 45 -> 6 of 691,200, and presented frame 299 is exact. The one remaining frame-300 pixel — source `(118,209)`, native `(11,0,15)` versus retail `(11,0,16)` — is below the evidence bar and is not tracked as work. This step now advances on visible correctness across reachable gameplay and UI scenes; widescreen and interpolation are no longer gated on a difference count.
- notes: PID 3898998 verifies Authored ordering restores the retail red letter winner. The separate hole investigation binds PSX packet `0x800C5394` to native object `0x800A0C74`/frame `0x200B`/face261/material02FB. Source vertices, transform, packet/native SXY and material match; native alone used third SZ while retail `0x800193A8` executes AVSZ3 and reads OTZ. Exact PID 4054917 at `138ad0a-dirty+psxport-ff21584d` proves retained-super `0x80033494` publishes ZSF3=341, face261 yields OTZ1511/sort1755, is accepted and writes display `(234,181)`. Output `(439,543)` changes from black to `(66,8,90)` versus PSX `(66,0,90)`, closing the large angular holes with the logo intact and no depth bias, skip, or title-local sorter. For issue 0015, packet `0x800C84D4` maps to object `0x801E1DB8`/frame `0x2008`/face9/materialA1B2 with exact packet/native SXY and exact captured DPCS colors. The absolute-VRAM write chain proves the correct writer/blend; the differing source texel isolated framework UV rounding. Packet `0x800C397C` binds the gradient witness to untextured G3 with exact SXY/colors and DTD off, isolating ordinary-shader quantization. Packet `0x800C89EC` then binds source `(85,137)` to exact face27 geometry/colors: PSX includes the vertex and unshifted Vulkan misses it, isolating untextured sample phase. The reachable shipping GPU selftests pass 28/28 UV-phase, 16/16 blend, 3/3 modulation, the textured integer-pixel coverage edge/interior probe, and exact G3 interpolation/DTD/edge probes; the textured half-pixel variant was first falsified by a 15,053-pixel regression, which `0e6c7e5d` explains and supersedes: that attempt kept the corner-relative UV rewind and predated the modulation fix. Packet `0x800C7454` then binds source `(238,61)` to a dark textured face whose retail texel `0x8421` modulates to zero and leaves background `0x0C01`, isolating rounded Vulkan modulation; reverting only the two shaders makes the new discriminator report the other answer in all three cells.

### graphics.enhancements — Enable widescreen and interpolation on PC-owned producers
- status: todo
- deps: graphics.camera-submitters
- evidence:
- where: game/render/
- gap: Requires verified native ownership of camera, projection, and transform producers.
- notes:

### graphics.render-anchors — Inventory retail projection and camera-control anchors
- status: re-verified
- deps: boot.recompile, boot.loaded-modules
- evidence: tools/inventory_render_anchors.py binds each answer to the shipping recomp-bootstrap input/output hashes, recompiler version, parsed denominator, and exact psxport commit. Previous pin 17981527 yields 1,724 functions/31 projection anchors/17 camera-control anchors; substrate baseline 99a42aa3 yields 2,005/31/17, with three named address differences but the same resident 0x80015780 -> 0x8001CD04 -> {0x800193A8,0x8001AF2C} chain. Its 12/12 selftest proves the positive/predecessor and GTE-only/memory-only/provenance/denominator negative answers. Current pin 784e5212 changes only presentation/watchdog lifecycle ownership and retains that generated substrate.
- where: tools/inventory_render_anchors.py, docs/findings/render-anchor-inventory.md
- gap: Runtime attribution after the clean CDC/completion gate must select the executed camera and submitter ancestry, then semantic decompilation must trace it back to game-state inputs.
- notes: Static GTE output is an RE locator only. It is not a native producer and cannot be used as picture input.
