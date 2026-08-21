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
- evidence: tools/provision.py verified the real USA CHD: SYSTEM.CNF boot 1/1 and executable identity/header 11/11 at SHA-256 fd5727a18feb2a2d5a6359a55966f0266284d1e50f64ee9b8a127a97091bd516; tests/test_provision.py passed 8/8 positive, mismatch, refusal, precedence, ambiguity, and atomic-publication tests through the shipping provision path, including one failing case for each of the 11 manifest facts.
- where: tools/provision.py, tests/test_provision.py
- gap:
- notes: The disc and provisioned executable remain external/gitignored. This proves identity and provisioning only, not recompilation or boot.

### boot.recompile — Generate the static-recompilation substrate
- status: re-verified
- deps: boot.provision
- evidence: The verified USA image plus measured indirect targets and the retail-derived HookEntryInt setjmp re-entry emitted 340 roots into 908 resident functions over 0x80010000..0x80079000, with zero configured overlays. tools/recomp_bootstrap.py passed 6/6: the real positive plus changed BSS-bound, out-of-text seed, one-byte executable mutation, wrong re-entry, and duplicated main/main_reentry refusals. A Clang build linked the output and dispatched 0x80031AE8 without a recomp miss.
- where: tools/recomp_bootstrap.py, game/recomp_seeds.json, game/core/game_config.cpp, game/core/recomp_register.cpp, generated/ (gitignored)
- gap:
- notes: Generated output is rebuilt from the verified executable and remains untouched, gitignored, and non-authoritative. The main_reentry seed is mechanically tied to ResetCallback 0x80031A80: its sole jal to setjmp 0x8003ACEC resumes at call+8, 0x80031AE8.

### boot.harness — Establish a deterministic psxport/oracle boot comparison
- status: re-verified
- deps: boot.recompile
- evidence: The independent Beetle interpreter ran the actual USA CHD for 600 frames and repeatedly showed the saved-context path 0x80031AE8 (v0=1, sp=0x80068B14) -> 0x80031B58 (same sp, ra=0x80031AF8). The Clang port shared that prefix and continued through IRQ2 callback 0x8003F5F0 -> drain 0x8003E14C with no recomp miss or old CD timeout. tools/verify_oracle_irq.py passed the real comparison and 4/4 selftests; tools/verify_boot.py passed 5/5 and requires file-load progression after the PVD lookup.
- where: tools/verify_oracle_irq.py, tools/verify_boot.py, game/recomp_seeds.json, game/core/
- gap:
- notes: The comparison is deliberately at the custom exception-exit boundary shared by both machines. It does not claim full-RAM lockstep or that the later CD filesystem state agrees.

### boot.first-divergence — Reach and root-cause the first real divergence
- status: re-verified
- deps: boot.harness
- evidence: The shared BFRD latch preserves Crash Bash's split PVD read: at the 512-word DMA, LBA=16, data_rd=12, data_n=2340, and the next bytes are 01 43 44 30 30 31 01 00 (type 1 + CD001). The game finds CRASHBSH.DAT and prints load file start. The continuous-event path then delivers LBA35800 after LBA35799 DMA3+DMA512 leaves the 280-byte tail, and the game advances through LBA35987 with matching header/payload DMAs and no recomp miss. It then restarts the same 189-sector range instead of printing done loading.
- where: external/psxport/runtime/recomp/cdc_native.cpp, tools/verify_boot.py, docs/issues/0005-crash-bash-iso-lookup-reads-the-pvd-payload-from.md, docs/issues/0006-crash-bash-continuous-readn-stops-after-the-firs.md, docs/issues/0007-crash-bash-repeats-the-189-sector-crashbsh-dat-r.md
- gap: Compare the read-completion/result state at 0x80027790, 0x80027944, and 0x800348A8 after LBA35987 with the true oracle; also make the shared watchdog count verified CD/FMV progress.
- notes: The current watchdog stop occurs during active CHD decompression and cannot prove a guest freeze. Disabling it or adding a game-local file-read override would hide the remaining result-state divergence.


## graphics

### graphics.camera-submitters — Identify native camera state and graphics submitters
- status: todo
- deps: boot.first-divergence
- evidence:
- where: game/render/
- gap: RE from game-state inputs before creating native producers.
- notes: Do not derive pictures from GTE, OT, or GP0 output.

### graphics.enhancements — Enable widescreen and interpolation on PC-owned producers
- status: todo
- deps: graphics.camera-submitters
- evidence:
- where: game/render/
- gap: Requires verified native ownership of camera, projection, and transform producers.
- notes:
