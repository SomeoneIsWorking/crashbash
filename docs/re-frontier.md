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
- status: todo
- deps: boot.target
- evidence:
- where: tools/
- gap: Add CLI > environment/.env > drop-in disc resolution and hash-check the extracted SCUS_945.70 against the tracked identity.
- notes: Never track the disc or extracted executable.

### boot.recompile — Generate the static-recompilation substrate
- status: todo
- deps: boot.provision
- evidence:
- where: generated/, game/recomp_seeds.json
- gap: Derive seeds from the measured executable and emit generated code; generated output remains gitignored and sacrosanct.
- notes:

### boot.harness — Establish a deterministic psxport/oracle boot comparison
- status: todo
- deps: boot.recompile
- evidence:
- where: tools/, game/core/
- gap: Build the game seam and an oracle that demonstrates both agreement and deliberate disagreement before porting logic.
- notes: A framework smoke build is not a game boot.

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
