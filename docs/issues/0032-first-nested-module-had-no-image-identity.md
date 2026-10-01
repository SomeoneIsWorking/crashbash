---
id: 32
title: The first nested gameplay module had no image identity at all, which is where the product actually stopped
status: resolved
symptom: docs/project-state.md records the product reaching only f255 in one process state; a longer
  run reaches the attract flow, loads DAT28272, and then the executor refuses guest address
  0x800C3434 as "ambiguous code-image identity" and the run ends. The follow-on gap — "the product
  never leaves process state 0x8004E0B8" — was a DEAD TAP plus an unpressed pad
state_items: S003, S004, S008
tags: image-identity,module-publication,cd-file-read,frontier,probe
created: 2026-09-27
updated: 2026-09-30
---

## The recorded frontier was a budget artefact, not the boundary

`docs/project-state.md` records a 400-frame run ending at f255, dwelling in `0x8004E0B8` for 256
frames, and correctly says "reaches f255 is NOT progress". The frame budget was the limit, not the
product. Raised to 1500 frames the same build completes the boot logo sequence and keeps going; the
debug server, which lifts the cap by design (`native_boot.cpp`: "When the debug server is up
(headless, no REPL), the run is INTERACTIVELY DRIVEN over the socket — do NOT cap it"), carried one
run to 37,506 presented frames and 21 authenticated image generations.

## The real boundary, and it is a correct refusal

    [native-dispatch:error] guest address 0x800C3434 resolves to zero or multiple active code images
    [executor:error] Crash Bash process update required a completed guest call, but execution
      exited as fault at 0x800C3434 after 251040 cycles: ambiguous code-image identity

The refusal was right. `crashbash-cd native file read completed: 38 sector(s) from LBA 28272 to
0x800B32B4` is `titles/crashbash/dat28272_module.json` exactly (`payload_disc_lba` 28272,
`sector_count` 38, `load_address` 0x800B32B4), and `cdFileReadOwned` had already done the correct
thing with it: `retireImagesForCdSectorWrite` removed BOOT's authenticated coverage over the bytes
it rewrote. Nothing then published the new module, so the range had **zero** image identity and
`resolveHostDispatch` step 3 turned it into a typed fault. Two facts made the gap:

- `runtime::GuestImage` had no value for DAT28272, DAT28241 or DAT28382, so a native key for them
  could not even be named.
- `cdFileReadOwned` offered each read only to `completeBootImageRead` / `completeMenuImageRead`.
  The codemap had already flagged this: "Later nested modules belong at this title-owned
  publication boundary."

## 0x800C3434 is the module's initializer, read from the image

It is inside DAT28272's payload range `0x800B32B4..0x800C62B4` and opens by installing its own
behaviour table:

    800C3434  addiu $sp, $sp, -0x18
    800C3438  lui   $v1, 0x8006
    800C343C  lui   $v0, 0x800c
    800C3444  sw    $v0, -0x5590($v1)      ; 0x8005AA70 — the address docs/project-state.md
                                                 already attributes to DAT28272

BOOT also stores the word `0x800C3434` at `0x80098FEC`, so the resident module table reaches the
initializer directly.

## The entry-word witness does not exist for this module family, and that is measured

`AuthenticatedModuleSpec` demanded a second content witness: the word at `entry_pointer_offset`
must equal `entry`. BOOT records offset 0 -> `0x80092BDC` and MENU 0x6270 -> `0x800B5244`. The five
nested overlays record `null` for both, and that is a property of the images: DAT28272's first eight
words decode as ASCII — `42 52 45 41 54 48 45 00` = "BREATH", `4A 55 4D 50` = "JUMP", `44 41 5A 45` =
"DAZE", `57 49 4E 00` = "WIN" — the animation-state name table `docs/project-state.md` already
describes for this module. Reading four bytes of a name table and comparing them against 0 would be
a guaranteed mismatch and a refusal that says nothing about the image.

So `hasEntryWitness(spec)` is now a stated property of the specification, `validModuleSpec` requires
the entry only when one was recorded, and both the success and the refusal log name how many
witnesses were used. The SHA-256 over the whole payload remains the content authority in every case.

## The fix

`game/core/nested_module_image.{h,cpp}` holds the five tracked nested specifications, built from the
same CMake-derived identity headers provisioning authenticates, and offers a completed read to the
ONE existing implementation `completeModuleImageRead`. No second digest, retirement, invalidation or
binding was written. `cdFileReadOwned` calls it beside the BOOT and MENU offers.
`GuestImage` gained `Dat28272`, `Dat28241`, `Dat28382`.

## Measured, after

| measurement | 4:3 | 16:9 |
|---|---|---|
| guest calls completed | 2,354,715 | 2,354,715 |
| needing a resume | 60 (deepest 7 turns, cap 12) | 60 |
| executor calls | 2,497,655 | 2,497,655 |
| executed blocks | 304,095,272 | 304,095,272 |
| executed instructions | 4,740,055,996 | 4,740,055,996 |
| fallback blocks / instructions | **0 / 0** | **0 / 0** |
| faults | 0 | 0 |
| image generations published | 16 (boot 1, menu 10, dat28272 3, dat28241 2) | same |

The two aspects produce **identical** guest-execution telemetry, which is the property widescreen is
supposed to have: the picture changes, the simulation does not.

Live census from the long run's debug endpoint (37,506 presented frames, a different run from the
capped pair, quoted because it is further along):

    calls=4382380 translated_blocks=10584 executed_blocks=540076720
    executed_instructions=8442308567 host_dispatches=4209650
    cache_hits=540061054 cache_misses=15669 faults=0
    fallback: calls=0 instructions=0 refused_calls=0, and every reason 0

Zero interpreter fallback by any of `compilation_failed`, `self_modifying_code`,
`unsupported_block`, `load_delay_hazard`, `unsafe_instruction_fetch`. Gameplay execution is
nonzero-dynarec, so this is not interpreter-covered evidence.

## The "never leaves 0x8004E0B8" gap, resolved — the observer was a DEAD TAP, and no run had ever pressed a button

Two independent causes, both measured, and the first one invalidates the way the gap was stated.

### 1. `kAppModeVtable` cannot select a mode, and the mode machine was never being read

`0x8004E0DC` is the shell's OWN scene, not a mode selector. Measured from the authenticated images:

- Its single writer is `0x800101CC` — `sw $s0, -0x1f24($v0)` in the resident application main, with
  `$s0 = 0x80078C90`, which is **BOOT's load address**. That write happens once, before any mode
  exists, and installs BOOT's scene header (enter `0x80092BDC`, update `0x80092BA0`,
  present `0x80092B7C`).
- The object it points at has the retail scene shape `{enter, update, present, arena}`, and BOOT's
  own update is `func_0x8001e598(&DAT_8009f644); func_0x8001e610(&DAT_8009f658, &DAT_8009f644)`.

So the machine that selects boot / menu / attract / gameplay is the **scene machine at `0x8009F658`**
(`{current, target, previous, flags}`) with its transition clock at `0x8009F644`, driven by
`0x8001E610` and armed by `0x8001E588`. The frame driver watched the root scene pointer instead, so
"reached Crashball" and "never left the logo" printed the identical line. This is another dead tap in this
workspace's own table of them, and it is the first one here that was a *reporting instrument* rather
than a counter — which is why it survived every run: it printed a confident, well-formatted line
either way.

`crashbash_scene_machine_test` now derives this from bytes rather than from a note: it censuses every
`lui`+displacement `lw`/`sw` over all 107,520 resident text words and all 96,768 BOOT payload words
(the form a literal 32-bit address never appears in), asserts the writer count for `0x8004E0DC` is
exactly **1** at exactly `0x800101CC`, and pins BOOT's `lui/addiu` pairs and its `jal 0x8001E610` to
the clock and scene record. The negative case is in the same test: the same scan over `0x8009F658`
MUST find stores in BOOT, so the count of 1 is a property of that word and not of a scan that sees
nothing.

### 2. No recorded run had ever delivered a pad edge, so the attract loop was doing exactly what retail tells it to

The sequence `MENU → DAT28272 → DAT28241 → MENU → …` is the **attract cycle**, and Crash Bash's attract
loop is *supposed* to run until a button is pressed. Every run in this repository's logs, including
the 37,506-frame one quoted above, drove the pad with the host at rest: no `PSXPORT_PAD_REPLAY`, no
debug-server `press`. The BOOT-logo skip override
(`game/core/boot_logo_skip.cpp`) is the only thing that ever consumed a host button, and it only runs
during the logo — so its working proved the host mask was live and said nothing about the guest.

### The fix, in its owner

Nothing writes the scene record, the clock, the transition flags, the vtable, or any phase word. The
guest takes its own transition, exactly as retail does:

- `game/diagnostics/scene_machine.{h,cpp}` — a native override on the retail leaf `0x8001E588`
  (`sw $a1,4($a0); sw $a2,0xc($a0); jr $ra; sw $zero,8($a0)`) that reads the guest's arguments,
  calls the original through the scoped original-call path, and then reports what the machine did
  with the request: `current` and `target` before and after, plus the clock age. "Asked for X" and
  "entered X" are printed apart, because they are different claims.
- `game/core/crashbash_frame_driver.cpp` — reports the live scene (`0x8009F658`) on every change with
  no cap, naming its enter/update/present, its previous/target/flags, and the clock age. The root
  scene line is kept and relabelled for what it is.
- input is delivered the way a player delivers it: a `PSXPORT_PAD_REPLAY` stream of real per-frame
  active-low pad masks, which is the guest's own SIO0 packet path — not a direct write to a button
  word.

### Measured, on the pinned Lightrec product

One 4000-frame headless run of the rebuilt tree, `PSXPORT_NATIVE_FRAMES=4000`, `PSXPORT_PAD_REPLAY`
carrying Cross held for 5 frames every 24 frames, `LUCENT_DEBUG=crashbash-boundary`,
`PSXPORT_PRESENT_SHOT_AT=100,140,700,1600,3000,3900`, exit 0, pad replay fully consumed
(`4000 of 4000 pad frame(s)`).

**The transition, taken by the guest.** The scene machine changed **9 times**, and every change is the
guest's own — logged by the frame driver's live-scene line, never written by the title:

| f | scene | enter / update / present | previous |
|---|---|---|---|
| 2 | `0x800A00DC` | `0x800936C4` / `0x80093BE8` / `0x8009407C` | — |
| 3 | `0x800B9524` | `0x800B5244` / `0x800B5218` / `0x800B51EC` — **the MENU scene** | `0x800A00DC` |
| 27 | `0x800A00DC` | as above | `0x800B9524` |
| 79 | `0x8009F720` | `0x80092CAC` / `0x80092EDC` / `0x80092E94` | `0x800A00DC` |
| 303 | `0x800A0BF4` | `0x80094754` / `0x80094B38` / `0x80094EB8` | `0x8009F720` |
| 318 | `0x8009E5C8` | `0x8008EF9C` | `0x800A0BF4` |
| 319 | `0x800A00DC` | as above | `0x8009E5C8` |
| 359 | `0x8009F720` | as above | `0x800A00DC` |

and the request that produced the menu handoff is the guest's own, observed on `0x8001E588`:

    [crashbash-scene] guest requested scene 0x8009F658 -> 0x800A00DC request-flags=0x12:
      current 0x800B9524->0x800B9524 target 0x00000000->0x800A00DC previous=0x00000000
      record-flags=0x12 clock-age 27->27

(the `current` is unchanged on the request line and has moved by the next frame's scene line — which
is exactly why the observer prints the request and the entry apart).

**The Cross edge and the controlled mode:**

    [crashbash-boundary] MENU accept edge=00004000 current=800B8E28 pending=00000000->800B8E50 selection=00000000
    [crashbash-module] authenticated crashbash-usa-dat28136 image generation 5 at 0x800B32B4: 42 sector(s), 86016 byte(s)
    [crashbash-boundary] DAT28136 registration addr=800B4E1C ra=80078D10 callback=80093038->800B4694

**Denominators for that run:**

| measurement | value |
|---|---|
| executor calls | 873,988 |
| executed blocks | 53,768,141 |
| executed instructions | 840,634,226 |
| memory callbacks / device clock commits | 251,957,238 / 963,697 |
| fallback blocks / instructions | **0 / 0** |
| fallback reasons | `compilation_failed=0 self_modifying_code=0 unsupported_block=0 load_delay_hazard=0 unsafe_instruction_fetch=0`; `refused_*` identical |
| faults | 0 |
| guest calls completed / needing a resume | 851,945 / **6**, deepest **7** host turns against the derived cap of 12 |
| image generations published | 3 — BOOT gen 3, MENU gen 4, DAT28136 gen 5 |
| scene changes | **9** |

Nonzero dynarec execution and **zero** fallback by any reason, so this is not interpreter-covered
evidence.

**The picture.** `scratch/screenshots/present_100.png` is the Cross-driven main menu
(`SELECT GAME TYPE / ADVENTURE MODE / BATTLE MODE / TOURNAMENT / OPTIONS`), and `present_140.png` is
the next page of the same chain (`SELECT NUMBER OF PLAYERS / 1 PLAYER / 2 PLAYER`) — both reached by
Cross alone. `present_700`,
`present_1600` and `present_3900` are the Crashball character-select stage, each
**689,194 / 691,200 non-black (99.71%)**, with the guest's own model draws behind it (80 accepted
pre-GTE draws, 4,922 faces captured, 2,917 textured, ~2,817 submitted per frame).

### What the attract loop was, and what a run without input still is

An idle run still dwells in `0x8004E0B8` forever and still reloads MENU and the attract modules. That
is not a defect and the log now says so: the scene lines name the live scene, and a run that never
changes it is visibly a run that never left the logo.
- **SUPERSEDED 2026-09-30 as a gate — this hazard is no longer on the widescreen path, and this
  paragraph must not be read as a blocker.** The measured content stands: the projection H the model
  producer widens is `camera + 0x18` where `camera = *0x800569E0`, and the horizontal scale is
  `*0x8005B698 + 4`; both sit behind pointer globals, so neither has an address to scan, and each
  pointer has exactly 2 writers, all resident (`0x80018C3C`, `0x80029C94` and `0x80027590`,
  `0x8002768C`). What changed is the consequence. Under the corrected sourcing rule (project-state
  S005) the port sources margin objects from **object memory**, ignoring the game's culls, and
  **widens no guest window at all** — so there is no guest scalar to be gameplay-unsafe. The owner
  already worked host-side: `projection.ofx += margin << 16` on the captured transform, with the
  authored-screen draw area clamped to the centred viewport. Answering the hazard by observing stores
  on the camera struct is still a worthwhile fact about the title; it is no longer a precondition for
  the feature, and "not answered" here no longer means "unsafe to ship".

### 3. Follow-on, 2026-09-30: the route was wrong, not just the observer — an interactive live match

The section above got to Crashball's character-select stage and stopped there. That was not the guest
refusing to continue: **the input shape was wrong, and the wrongness is only visible from the
retained replay.** The tracked `replays/flow/crashball-control.pad` holds **Circle for 800 frames
(1200-1999)**; the leg above tapped Cross on a 24-frame cadence and nothing else. On character
select, Cross cycles the roster, so a Cross-only replay reaches that stage and then sits there
indefinitely — and "sits there" looked exactly like the mode refusing to start.

The tracked replay, driven unchanged on the dynarec product, reaches a **live Crashball match**:
3,740 frames, exit 0, replay consumed 3,740 of 3,740. Route: attract exit → MENU → DAT28136
(character select) → **DAT28241 published at frame ~2000**, which is the module that runs the match.
Note what that does to the scene record: at the shared `0x800B32B4` slot, address `0x800B4694` is
DAT28136's registered callback in image generation 5 and DAT28241's code in generation 6, so the
scene's `update` pointer does not change while the code behind it does. That is the loaded-image
identity case S015 measures, and it is why "the scene never changed" was never going to be the signal.

| measurement | value |
|---|---|
| executor calls / blocks / instructions | 824,429 / 56,755,797 / 881,471,499 |
| fallback blocks / instructions | **0 / 0** |
| fallback reasons | all five `=0`; `refused_fallback_blocks=0`, all five `refused_*` `=0` |
| guest calls completed / needing a resume | 803,814 / **9**, deepest **7** host turns against the derived cap of 12, 14,820,784 cycles over those calls |
| image generations published in one run | **4** — BOOT 3, MENU 4, DAT28136 5, DAT28241 6 |
| scene changes | **10**, all guest-taken through `0x8001E588`; the observer is read-only |
| presented match frames | 553,189-557,800 of 691,200 non-black (80.03-80.70%) |
| per-frame host time, unpaced | **p50 9.50 ms, p95 13.25 ms, p99 18.50 ms, worst 359.77 ms, 3 beyond range** |

**Movement is proven against a control leg, not asserted from a screenshot.** The control is the
byte-identical replay with every frame from 3560 on forced to `0xFFFF` — 180 frames changed, all
other frames equal — so it receives no direction at all. Full 2 MiB guest-RAM dumps at 8 frames in
both runs give:

- frame 3555, before any held direction, differs in **0 of 524,288** words between the legs: the dump
  is deterministic and the legs are the same run up to the input;
- the guest's parsed P1 word `0x80063A92` reads `0xFF7F` under held Left and `0xFFDF` under held
  Right in the treatment, and **`0xFFFF` in the control at all 8 frames**;
- of 524,288 words, **159** fall monotonically across all three Left steps and rise monotonically
  across all three Right steps, and every one of them also differs from the control at 6 of the 7
  post-input frames. `0x8005721C` is the clearest: `118, 113, 101, 99` then `95, 98, 99, 116`;
- the HUD score advances during the window (P1 `12` at 3560 → `11` at 3640 → `09` at 3730 as the
  ball reaches the left goal), so ball and score state are live rather than frozen.

**What is NOT resolved, stated so it is not re-derived as if it were:** the individual store
instruction behind those words. `tools/probe_addr_refs.py --stores-only` returns **0 sites** for
`0x80056ACC`, `0x80056ADC`, `0x8005721C`, `0x801D4048` and `0x801D40FC`, and no 32-bit word in the
dumped RAM equals any of them or a plausible base, because the guest computes the address as
base+index in a register. The one base in that band the images materialise by `lui` is `0x80056998`
(BOOT `0x800825B4` and `0x80082624`, `lui $v0, 0x8005; addiu $s0, $v0, 0x6998`) inside the
input-edge handler that tests direction bits `0x4000` and `0x40`. Resolving the loop that walks that
base is the next RE step and is the open half of this issue.

**A tenth dead tap, found in the instrument rather than the game.** Nothing in the title called
`Game::perf.frameBegin()/frameEnd()`, so the framework's per-frame profiler produced no timing line
at all — no p50, no p95, nothing — for this title, on a passing framework test and a working
channel. Absence of output read as absence of a measurement. It is now bracketed in
`CrashBashFrameDriver::stepFrame`, and the distribution above is from it. The channel knob is
`PSXPORT_DEBUG` (`cmake/psxport.cmake` sets `LUCENT_CHANNEL_ENV=PSXPORT_DEBUG`); an earlier leg of
this work set `LUCENT_DEBUG`, which does nothing here — the channels that appeared to be enabled
were emitting only because they are info-level.
