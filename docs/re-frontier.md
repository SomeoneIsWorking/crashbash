# RE frontier

Tracked with the shared `re_frontier.py`; this is the ordered dependency chain from the real disc toward
a faithful port. `re-verified` requires executable evidence plus a real verification, never merely a
successful build.





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
- evidence: tools/provision.py verified the real USA CHD: SYSTEM.CNF boot 1/1, executable identity/header 11/11, and two measured CRASHBSH.DAT code modules 14/14. tests/test_provision.py passes 11/11 positive, mismatch, refusal, precedence, ambiguity, shared-source, bounded-offset, and atomic-publication tests through the shipping provision path, including one failing case for each of the 11 executable facts and a mutation of either loaded payload.
- where: tools/provision.py, tools/loaded_module.py, tests/test_provision.py, titles/crashbash/executable.json, titles/crashbash/boot_module.json, titles/crashbash/menu_module.json
- gap:
- notes: The disc, full 73,220,096-byte DAT, executable, and extracted modules remain external/gitignored. The tracked manifests prove identity and provisioning only, not game behavior.

### boot.recompile — Generate the static-recompilation substrate
- status: re-verified
- deps: boot.provision
- evidence: The verified USA image plus measured indirect targets, the retail-derived HookEntryInt setjmp re-entry, and two measured CRASHBSH.DAT code modules emit 1,063 roots into 1,724 functions: resident 0x80010000..0x80079000, BOOT 0x80078C90..0x800D7490, and nested MENU 0x800B32B4..0x800BB2B4. tools/recomp_bootstrap.py passes 9/9 real-positive and forced-negative identity/configuration/cache-integrity checks, and a Clang build links both retained generated overlay bodies.
- where: tools/recomp_bootstrap.py, tools/loaded_module.py, titles/crashbash/boot_module.json, titles/crashbash/menu_module.json, game/recomp_seeds.json, game/core/game_config.cpp, game/core/recomp_register.cpp, generated/ (gitignored)
- gap:
- notes: Generated output is rebuilt from the verified executable/modules and remains untouched, gitignored, and non-authoritative. The main_reentry seed is mechanically tied to ResetCallback 0x80031A80: its sole jal to setjmp 0x8003ACEC resumes at call+8, 0x80031AE8.

### boot.harness — Establish a deterministic psxport/oracle boot comparison
- status: re-verified
- deps: boot.recompile
- evidence: The independent Beetle interpreter ran the actual USA CHD for 600 frames and repeatedly showed the saved-context path 0x80031AE8 (v0=1, sp=0x80068B14) -> 0x80031B58 (same sp, ra=0x80031AF8). The current Clang port preserves that prefix and IRQ2 callback 0x8003F5F0 -> drain 0x8003E14C. Against exact recorded pin 784e5212, the serialized product completes both loaded-module requests, prints `empty prims`, then reaches measured MENU entry 0x800B5244 from ra=0x8001E7C0 without a watchdog stall or recomp miss. tools/verify_oracle_irq.py passes the real comparison and 4/4 selftests; tools/verify_boot.py passes the 72-line real boundary and 12/12 controlled answers.
- where: tools/verify_oracle_irq.py, tools/verify_boot.py, game/recomp_seeds.json, game/core/crashbash_runtime.cpp
- gap:
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
- gap: Invoke `tools/verify_cdc_phase_progress.py --port scratch/bin/crashbash_port --executable scratch/bin/crashbash/SCUS_945.70 --timeout 30` against the clean product; the tool drives and stops its exact child, timeout is refusal, and only positive LBA 17655 passes. A saved log requires `--trace PATH`, not a positional argument. In the same serialized investigation, capture 0x800348A8 / 0x80027944 returns and require the oracle's guest-visible result 1 -> 0; the earlier candidate's 5/5 distinct Pause response edges rule out the old controller-level coalescing mechanism in that trace but do not prove the current pin. Preserve the deterministic sector schedule; do not add CDC/log-dispatch heartbeats to the watchdog.
- notes: No game-local file-read HLE, fake CdInit, or watchdog relaxation is permitted.

### boot.loaded-modules — Provision and execute the initial loaded code modules
- status: re-verified
- deps: boot.first-divergence
- evidence: Real CDC/DMA traces identify BOOT as CRASHBSH.DAT +0x04575800, 0x5E800 bytes, loaded at 0x80078C90 with entry pointer 0x80092BDC, and MENU as +0x03693000, 0x8000 bytes, loaded at 0x800B32B4 with callback pointer 0x800B5244 at +0x6270. Provisioning verifies the full 73,220,096-byte DAT plus both payload hashes and pointers (14/14 module facts); emission retains both generated bodies. The serialized exact-pin 784e5212 product gate completes both loads, prints `empty prims`, then its game-owned observer reaches MENU 0x800B5244 from ra=0x8001E7C0 before stopping the exact child, with no STUCK, fatal output, or recomp miss. tools/verify_boot.py passes 12/12 controlled answers and tools/verify_command_response_timing.py passes 5/5.
- where: tools/loaded_module.py, tools/provision.py, tests/test_provision.py, tools/recomp_bootstrap.py, tools/verify_boot.py, tools/verify_command_response_timing.py, titles/crashbash/boot_module.json, titles/crashbash/menu_module.json, game/recomp_seeds.json
- gap:
- notes: No DAT or module bytes are tracked. The shared CDC phase machine is landed and candidate-run; issue 0009 remains open for the process-driving clean CDC/completion gate and the resulting post-MENU no-present cause. No game-local CD HLE or renderer fallback is permitted.


## graphics

### graphics.camera-submitters — Identify native camera state and graphics submitters
- status: todo
- deps: boot.drive-timing, boot.loaded-modules, graphics.render-anchors
- evidence:
- where: game/render/
- gap: The corrected boot gate reaches MENU 0x800B5244 without the old false watchdog alarm but stops before a game frame; it therefore cannot answer render ancestry. Use the process-driving CDC/completion gate to identify the post-MENU no-present cause, then reach at least 50 presented frames with PSXPORT_DEBUG=rtpcaller, pass tools/verify_render_anchor_reach.py against the exact static inventory, and decompile the observed ancestry back to game-state camera and submitter inputs before creating native producers.
- notes: game/render/ does not exist and no graphics producer is registered. The static inventory narrows the first RE slice but does not prove runtime execution or camera semantics. Do not derive pictures from GTE, OT, or GP0 output.

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
