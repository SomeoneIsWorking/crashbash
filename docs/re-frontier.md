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
- evidence: The real USA image plus the one runtime-measured indirect IRQ target emitted 339 roots into 907 resident functions over 0x80010000..0x80079000, with zero configured overlays. tools/recomp_bootstrap.py passed 4/4: the real positive, changed BSS-bound refusal, out-of-text seed refusal, and one-byte executable mutation refusal. A Clang build linked the emitted interfaces and the live verifier reached guest main 0x8002718C and IRQ callback 0x8003B1BC without a recomp miss.
- where: tools/recomp_bootstrap.py, game/recomp_seeds.json, game/core/game_config.cpp, game/core/recomp_register.cpp, generated/ (gitignored)
- gap:
- notes: Generated output is rebuilt from the verified executable and remains untouched, gitignored, and non-authoritative. The IRQ seed is evidence-backed: guest CD sync 0x8003EDBC called VSync 0x800320EC, which dispatched the callback stored at 0x8006D98C; Ghidra confirmed that target is 0x8003B1BC.

### boot.harness — Establish a deterministic psxport/oracle boot comparison
- status: todo
- deps: boot.recompile
- evidence: The port-side boundary verifier passed its real run plus two forced negatives: removing the guest-main trace and injecting a recomp miss. The real run audited CRT0 10 agree / 0 disagree / 0 unresolved, reached guest main and the IRQ callback, and stopped at repeated CD timeout / unclaimed IRQ / watchdog-stuck diagnostics.
- where: tools/verify_boot.py, game/core/
- gap: Add the true oracle and compare deterministic state at this same boundary, then measure and implement the CD/VSync hardware/IRQ service instead of weakening the watchdog.
- notes: Port-side both-answer validation is established, but this step remains todo because no oracle run is yet compared.

### boot.first-divergence — Reach and root-cause the first real divergence
- status: todo
- deps: boot.harness
- evidence:
- where: game/core/
- gap: Advance only after the harness is validated on both answers.
- notes:

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
