# RE frontier

This is the ordered evidence chain from the authenticated retail images to the native/dynarec product.
The static product is not regenerated, built, or run during the migration. Existing static files are
removed only after the representative-gameplay gate; they are never a fallback or comparison oracle.

USER 2026-08-30: "Change the directive, pixel matching doesn't matter. I just want working game that
looks correct."

Running behavior and visible correctness establish the product frontier. Pixel and state comparisons
remain diagnostics for finding causes, not completion conditions.

## Runtime migration

### runtime.target — Authenticate the retail target

- status: re-verified
- deps:
- evidence: USA `SYSTEM.CNF` boots `SCUS_945.70`; SHA-256
  `fd5727a18feb2a2d5a6359a55966f0266284d1e50f64ee9b8a127a97091bd516`; PS-X EXE entry
  `0x8002E7B0`, load `0x80010000`, text `0x69000`; CRT0 extraction resolved 8/8 boot fields.
- where: `titles/crashbash/executable.json`, `titles/crashbash/README.md`
- gap:
- notes: Identity and CRT0 evidence only; no execution-engine claim follows from it.

### runtime.images — Authenticate every reached loaded image

- status: re-verified
- deps: runtime.target
- evidence: BOOT is at LBA 35799. MENU, DAT28272, DAT28241, DAT28136, DAT28382, and DAT22510
  occupy the reused nested region at `0x800B32B4`. DAT22510 is `CRASHBSH.DAT + 0x02B81000`, size
  `0x23000`, SHA-256 `fb8ee41f2c19a9c419e7b4240de82237fca09efee98bdfec63b7ec430a18457f`.
  The provision path validates the complete 73,220,096-byte DAT and 44/44 recorded facts.
- where: `tools/provision.py`, `tools/loaded_module.py`, `titles/crashbash/*_module.json`
- gap:
- notes: Runtime image generation is part of override and translated-block identity. No module bytes
  are tracked.

### runtime.lightrec — Execute the authenticated images through pinned Lightrec

- status: todo
- deps: runtime.images, psxport per-`Core` dynarec executor
- evidence:
- where: `external/psxport`, `psxport.pin`, `game/core/`
- gap: Wire the title to psxport's maintained, pinned Lightrec integration; prove nonzero translated
  blocks; audit the gameplay link and configuration surfaces to prove that the interpreter in the
  separately built test target, including diagnostics, is absent and cannot be selected or entered as
  a fallback.
- notes: No offline translator, generated function table, or generated corpus may participate.

### runtime.overrides — Preserve the native override boundary

- status: todo
- deps: runtime.lightrec
- evidence: The migration audit counts 27 current native override installations.
- where: title-owned modules under `game/`
- gap: Register all 27 by complete runtime image generation plus guest address, exercise enabled and
  disabled paths, and prove that overlay replacement cannot inherit a colliding resident/module hook.
- notes: Override changes invalidate any translated call path that captured the old decision.

### runtime.original-calls — Replace generated-body calls with scoped original calls

- status: todo
- deps: runtime.overrides
- evidence: The migration audit counts 15 calls from native owners to generated guest bodies.
- where: title-owned native overrides; psxport runtime executor API
- gap: Replace all 15 with the single runtime original-call operation. It suppresses only the active
  override for one call, executes the original authenticated guest body through Lightrec, preserves
  guest ABI/state, and cannot recurse into itself.
- notes: An original call never selects a static body or interpreter.

### runtime.first-frontier — Re-establish the current reached path dynamically

- status: todo
- deps: runtime.original-calls
- evidence: Recorded binary facts reach MENU `0x800B5244`; Cross schedules `0x800B8E50`, loads
  DAT28136, registers `0x800B4E1C`, replaces callback `0x80093038` with `0x800B4694`, and executes
  the update. The independent oracle records ResetCallback/setjmp resume
  `0x80031A80 -> 0x8003ACEC -> 0x80031AE8` and IRQ2 `0x8003F5F0 -> 0x8003E14C`.
- where: `docs/findings/vsync-owner-map.md`, `docs/issues/`, `tools/verify_boot.py`,
  `tools/verify_menu_accept.py`
- gap: Reach the same loaded-image and menu frontier through nonzero Lightrec execution, with all
  reached native owners active, bounded executor exits, correct invalidation, and no guest-VSync
  violation or wrong-image dispatch.
- notes: First frame, logos, and menu entry are implementation discriminators, not representative
  gameplay conformance.

### runtime.gameplay — Prove representative interactive hybrid gameplay

- status: todo
- deps: runtime.first-frontier, flow.pad-input, graphics.native-state
- evidence: Retained acceptance inputs reach live Crashball (3,740 frames), Battle Crate Crush
  (15,401), Tournament Crate Crush (7,910), and Polar Push (17,682), each with observable player
  movement and complete native presentation.
- where: `replays/flow/`, native render owners under `game/render/`, independent emulator diagnostics
- gap: Re-run the bounded scenarios on the native/dynarec gameplay product with nonzero Lightrec
  execution, correct timing/interrupt/device state, rendering, audio, and declared frame-time evidence
  on each released host architecture.
- notes: The interpreter in a separately built test target, including diagnostics, may diagnose a first
  divergence; it is not linked into or selectable by this product.

### runtime.retire-static — Delete the static execution path

- status: todo
- deps: runtime.gameplay
- evidence:
- where: product build/provisioning/launch surfaces and static-only artifacts
- gap: After representative gameplay passes, delete the offline translator, generated corpus, static
  dispatcher, seed-only metadata, generated-symbol checks, and stale methodology together. Prove a
  fresh checkout provisions, builds, and launches directly from the authenticated user image.
- notes: Do not build or run the old path while waiting for this gate. Deletion must leave no legacy
  mode, fallback, selector, or tombstone.

## Preserved retail and native boundaries

### flow.pad-input — Deliver host pad state through the retail SIO path

- status: re-verified
- deps: runtime.images
- evidence: Class 2 element `0x8006D984`, verifier `0x8003B1BC`, and handler `0x8003B224` own the
  VBlank/SIO chain. Idle/START A/B changes packet `0x80077FBC`, parsed P1 `0x80063A92`, and active-high
  P1 `0x8005133C` while P2 `0x80051394` remains unchanged.
- where: `docs/findings/crashbash-pad-sio.md`, shared psxport SIO/timing owners
- gap:
- notes: Direct guest-buffer injection is rejected.

### flow.menu-cross — Preserve the measured Cross transition

- status: re-verified
- deps: flow.pad-input
- evidence: Active table `0x800B8E28` updates through `0x800B3CA8`; rising-edge `0x80051380`
  accepts Cross `0x4000` at `0x800B3D88-0x800B3D8C` and schedules `0x800B8E50` through
  `0x8009F8A8`.
- where: `game/diagnostics/menu_boundary.cpp`, `docs/issues/0020-active-menu-was-incorrectly-expected-to-accept-start.md`,
  `docs/issues/0021-cross-menu-acceptance-needs-an-idle-start-cross.md`
- gap:
- notes: START is not an alternate acceptance route.

### graphics.native-state — Preserve native game-state rendering inputs

- status: in-progress
- deps: runtime.images
- evidence: Projection anchors `0x800193A8`/`0x8001AF2C` descend from decoder `0x80019A60`,
  submitters `0x80019F1C`/`0x8001DD50`, transform `0x8001965C`, source decode
  `0x8001C1E0`/`0x8001C0F0`, and prefill `0x80017EE8`. Sprite owners `0x8002992C`,
  `0x80029D28`, and authored-screen `0x8001A0D8` consume source descriptors. `0x8009440C`
  proves H is `camera + 0x18`; `0x80018B08` is setup rather than a drawable.
- where: `game/render/`, `docs/findings/crashbash-packet-pools.md`,
  `docs/findings/render-anchor-inventory.md`
- gap: Complete visible 4:3, widescreen, and interpolated coverage across the representative gameplay
  scenarios without deriving product geometry from GTE/OT/GP0/framebuffer output.
- notes: Existing native owners retain their semantics; where they need the original guest behavior,
  they use the scoped dynarec original-call boundary from `runtime.original-calls`.
