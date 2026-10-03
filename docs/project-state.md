# Project state

## Comparison baseline

The baseline is the unmodified USA PlayStation release of *Crash Bash* running on original hardware
or through a PS1 emulator, with retail game modes, a 4:3 camera, 30 Hz presentation, and console CPU
execution. The intended product authenticates the user's game image, executes selected behavior in
native title owners, dynamically translates all remaining MIPS through psxport's pinned Lightrec
revision, widens the camera, and adds 60 Hz interpolated presentation without accelerating simulation.

## Current focus

**S016 through S003, then S013.** Crash Bash runs in parallel with the active Spyro 1 title on the
native/dynarec product; nothing it lands may regress Spyro 1's gates. Finish list, in order:

1. **S016 remainder**: the live Crashball match is reached, controlled and measured (S016 above).
   What is left on this goal is audio/timing coverage from the headless WAV sink and per-frame
   frame-time evidence on the other released host architectures.
2. **Issue 0032's remaining citation**: the store instruction behind the measured movement words,
   which the images reach by base+index that no `lui` materialises. The base in that band is
   `0x80056998` (BOOT `0x800825B4` / `0x80082624`).
3. **Issue 0030's remaining half**: the resume contract is implemented and classified (0029); what is
   left is the framework-side negative case for a resume whose image generation has been retired.
4. **S003 ledger**: complete image publication for every resident and nested module on one
   representative run. The Crashball leg now publishes BOOT, MENU, DAT28136 and DAT28241 in one run;
   DAT28272, DAT28382 and DAT22510 have not been seen alongside them.
5. **S016 / S009–S012**: done 2026-10-02 for Tournament Crate Crush and Polar Push (S011, S012);
   Battle Crate Crush plays through live menu input (S013 note).
6. **S015**: all 28 overrides by image identity and the 16 original calls under the differential. The
   live-match leg is already a concrete instance of the case: `0x800B4694` is DAT28136's registered
   callback in one image generation and DAT28241's code in the next, at the same address.
7. **S013**: the remaining modes. **S014**: audio requalification.
8. Then requalify 60 fps interpolation. Widescreen is no longer gated on the camera-scalar hazard
   measurement: under the corrected sourcing rule (S005) no guest window is widened, so that
   measurement is off the critical path.

## Capability inventory

| ID | Capability / observable outcome | State | Dependencies | Goals |
| --- | --- | --- | --- | --- |
| S001 | The selected USA disc, executable, and measured `CRASHBSH.DAT` modules are reproducibly authenticated and provisioned | verified | — | G001 |
| S002 | The retail boot and loaded-image sequence have a recorded first-frame and menu frontier to re-establish through the dynarec | partial | S001, S003 | G001 |
| S003 | The gameplay product executes every non-native guest path through psxport's pinned Lightrec dynarec with bounded, reason-accounted fallback | partial | S001, shared psxport executor | G001 |
| S004 | Crash Bash graphics are produced natively from decoded game state and look correct across representative content | partial | S002, S015 | G001, G002, G003 |
| S005 | The native camera supports wider aspect ratios without changing vertical framing, with every title-owned horizontal cull or screen-rect limit overridden natively so the margins show what the view would: margin objects drawn from object memory, margin-only objects animated port-side, guest memory untouched | partial — the margin census against a 4:3 run is DONE (0 margin objects absent from object memory; guest RAM 524,288/524,288 words identical at frames 2500 and 2800; 12 of 12 census frames match). The cull/limit owner audit is partial: `0x80056ACC` has 3 store sites (resident `0x8001C0C0` and `0x8001C1B8`, both halfword stores off the source-decode base from `0x8001C008`/`0x8001C0F0`, plus `0x8001E188` off `0x8001A918`) and `0x80056ADC` has 2 (`0x8001AC78` off `0x80019CF8`, `0x8001E1C4` off `0x8001A918`), but the pod/movement words `0x801D4048`, `0x801D40FC` and `0x8005721C` return **0** sites because the guest forms base+index, and the only base in that band the images materialise by `lui` is `0x80056998` (BOOT `0x800825B4`/`0x80082624`). Battle, Tournament and Polar Push are unsurveyed | S004 | G002 |
| S019 | Widescreen anchors the UI: edge HUD elements sit at the widened edges or safe area, centred elements stay centred, nothing stretches | partial — **2026-10-02:** the per-player 3-digit score (BOOT HUD routine `0x800798A4`, digit sites `0x80079EE0/E6C/DF8`) is now classified with the portrait row, so Pogo Painter's scores stay under their portraits; still open — Polar Push's power bars and Crate Crush's wumpa bars are `RQ_OM_DEPTH` model geometry (submitter `0x80019F1C`), so the 2D anchor never sees them (2026-10-02 measurements). the anchoring policy, its per-element classification and its test are DONE and the arithmetic is verified per element (`PSXPORT_DEBUG=uihud`: `anchor=left-edge authored=(32,64) frame=512->684 margin=86 correction=-86`, right-edge `+86`, centred `0`). The 16:9 live-match capture now shows exactly **four** portraits and **four** score groups at the widened edges (P1 sink 55..135 = native 39, P4 sink 825..890 = native 588 = authored 416+172) with the countdown and lives flag still centred, where the pre-fix picture showed eight. That needed the FRAMEWORK seam `GameRuntime::guestPacketPoolWindows()` (psxport `6b5cee55`): without it `OtAttr` was structurally blind for a typed runtime, `GuestPacketFilter` matched nothing, and the guest's own GP0 replay drew a second, centred copy of every HUD element. Two gaps remain: **a 4:3 LIVE-MATCH capture is owed** (the tracked replay's route is host-timing dependent and the 4:3 leg now takes a different route), and **4:3 is not byte-identical to before** — suppression is aspect-independent, so the guest's second copy is gone there too; the two copies coincided in position but not in rasterisation (~34 px of digit ink), so one drawer remains where two drew. See issue 0034 | S005 | G002 |
| S006 | Native camera and world transforms render between simulation ticks | partial | S004 | G003 |
| S007 | Deterministic diagnostics compare reached hybrid-product boundaries with independent retail behavior and prove both answers | partial | S001, S003 | G001, G002, G003 |
| S008 | The retail game modes are reachable and playable end to end on the hybrid product | partial | S002, S003, S004, S015 | G001 |
| S009 | The Crashball gameplay scenario reaches a live match and accepts player control | verified | recorded behavior; **requalified on Lightrec 2026-09-30** — `replays/flow/crashball-control.pad` drives a live match, guest movement words are proven against an idle control leg, 0 fallback (S016, issue 0032) | G001 |
| S010 | Battle Mode Crate Crush reaches a live match and accepts player control | verified | recorded behavior; dynarec requalification in S016 | G001 |
| S011 | Tournament Mode reaches its first live Crate Crush match and accepts player control | verified | **requalified on Lightrec 2026-10-02** — re-recorded v1 `replays/flow/tournament-crate-crush-control.pad` reaches a live DAT28382 match and replays deterministically; from one saved match state the P1 record word `0x800AD304` holds -2082 for 18 idle frames and moves -2945 under Left then -1668 under Right (heading `0x801DD510` swings to ±4096), 0 of 524,288 words differ at the shared start | G001 |
| S012 | Polar Push reaches a visually correct, controllable live match | verified | **requalified on Lightrec 2026-10-02** — the match faulted `ambiguous code-image identity` at `0x800CBA64` because `dat22510_module.json` said 70 sectors while retail reads 71 (sector 71 continues the module's halfword table), so DAT22510 never published; manifest corrected to 71 sectors / `0x23800` / sha256 `7477dd74…`, re-recorded v1 `polar-push-control.pad` reaches the live match, P1 `0x800AD304` holds -2018 idle and moves -2800 under Left then -308 under Right | G001 |
| S013 | The remaining retail modes are reachable and playable | partial — **2026-10-02: Pogo Painter (Pogo Pandemonium) plays on Lightrec**: `replays/flow/pogo-painter-control.pad` reaches a live DAT28272 match and P1 paints squares under held directions (captured; the idle leg stays on its start square). On a fresh card Battle offers Crate Crush, Polar Push, Pogo Pandemonium and Ballistix; Tank Wars, Crash Dash and Medieval Mayhem show retail's own LOCKED page and need Adventure progress on the card, which is the next route. Earlier notes: every `replays/flow/*.pad` was a pre-v1 raw recording the runtime REFUSES, so S009-S012 had been running with the pad at rest; all four are now v1 and the Crashball route drives a live 4-player match again. Battle Mode is reachable and playable end to end (SELECT BATTLE TYPE -> briefing -> a live 4-player Crate Crush round with the player moving under held input), and the Adventure island carousel pans correctly across its signs. **The earlier per-character-availability theory for the island carousel is REFUTED by the console oracle**: on a settled island the reference holds the same `0x8005A6F8..0x8005A717` all zero, the same index `0x8005A677 = 0xFF`, the same `0x8005A618 & 0x2000 = false`, and shows no "press X" prompt, so the closed gate is retail's normal rest state and not our defect. `PSXPORT_WWATCH`'s reported `pc` cannot attribute a store (it is neither the block entry nor the store: the block at the reported PC contains no store to the watched range), and the store's own PC exists only inside lightrec's emitter-side store observer, not at the runtime `ops->sw` callback | S008 | G001 |
| S014 | Retail music and sound effects play at the correct rate without premature truncation | partial | S003 | G001 |
| S015 | All 30 native override registrations install by runtime image identity and all 18 original-body calls execute through the dynarec | partial | S003 | G001 |
| S016 | Representative interactive gameplay passes on the native/dynarec product | partial | S003, S004, S005, S006, S007, S008, S014, S015 | G001, G002, G003 |
| S017 | Every static product path is deleted before dynarec implementation and mechanically excluded | verified | — | G001 |
| S018 | Hosted CI truthfully covers applicable Linux, Windows, macOS, and Android product boundaries | partial | S003 | G001 |
| S020 | Crash Bash: load operations complete without loading-only waits or presentation; logos cancel through the recovered route | verified — no wait and no loading-only presentation: the guest's LOADING card draw (`FUN_80018B08`) is reached only from BOOT's two handoff screen presents, and the owner retires it at those two measured call sites while every other caller keeps the guest body; the authored transition clock is untouched, so the handoff fades through black into the next real picture (boot→menu 30 display fields, briefing→match 42, unchanged from S020's drain build, with zero card frames among them), and the keyed route still reports replay COMPLETE 1394/1394 into a controlled live Polar Push match | S003 | G004 |
| S021 | A whole-machine save state restores into any session (other minigame, menu, same match) with image identity, generations and native keys matching the restored RAM | verified — see 2026-10-02 measurements | S003, S015 | G001 |

### 2026-10-02 measurements

- **S004 menu arena previews — fixed, 4:3 and 16:9.** Cause: retail draws CHOOSE LEVEL / TOURNAMENT
  SELECT as several viewports (`0x80018B08` publishes the record at `0x800569A8`, a DR_ENV clip, and an
  OT slice at `0x800569D8`); the native model producer clipped to the present-time GPU area (full
  screen) and ordered by slice-relative key, so previews drew unclipped and behind the backdrop slice's
  black panels (screen quads, slice 2048 bin 128). Fix: models carry their viewport clip and slice
  (`render_viewport.h`); every world-ordered key and the sprite order is the OT word (slice + bin).
  Evidence: `scratch/preview/{v169,v43,tsheet,q-gte}.png`; gameplay unchanged in
  `scratch/preview/{pogo-live,polar43}.png`.
- **S021 cross-session save-state restore — fixed.** Cause: `MachineState::restore` replaced RAM but
  not the image catalog, generations, or native keys. Fix: title port `crashbash.image-identity` v1
  records every binding's surviving ranges; psxport `NativeStatePort::restored` re-publishes them as
  fresh generations after RAM. Evidence: polar→live polar at f6579 (`polar-into-polar.png`),
  pogo→polar and polar→pogo-session (`pogo-into-polar.png`, `polar-into-pogo.png`), 300 frames each,
  no fault; states without the section refuse by name, unmutated.
- **S019 HUD bars — open.** Polar Push power bars and Crate Crush wumpa bars are drawn through the
  model submitter `0x80019F1C` (BOOT `0x8001C2C0` cell list, vtable `+0x54`), so the 2D anchor never
  sees them. Unknown: the object argument the submitter receives for a bar (the `0x8009971C + i*0x498`
  cell pool matched 0 of 406 model objects; caller `0x80079C5C` is unclassified and was never on screen).
- **S013 locked minigames — not started.** Tank Wars, Crash Dash and Medieval Mayhem need Adventure
  progress no recorded route produces; their DAT payloads have no decoded file table.

The verified S009-S012 entries describe durable reached behavior and replay inputs, not dynamic-engine
completion. They become dynarec conformance evidence only after those scenarios run through the hybrid
gameplay product with nonzero Lightrec execution and the dynarec-first product audit.

## Capability details

### S001 — Authenticated retail inputs

Evidence: The USA disc resolves through `SYSTEM.CNF` to the exact `SCUS_945.70` identity, and tracked
manifests identify the resident executable plus all measured `CRASHBSH.DAT` images without tracking
copyrighted bytes.

### S002 — Boot and loaded-image frontier

Evidence: Recorded runs reach the first presented frame, MENU `0x800B5244`, the Cross-selected
DAT28136 registration/update boundary, and later interactive scenarios with the measured native boot,
device, and frame owners active.

The direct native/dynarec player authenticates the resident executable and completed 189-sector
BOOT and 16-sector MENU reads. It enters native frames 0 and 1, preserves untouched BOOT code when
MENU overwrites only part of it, and crosses the former MENU identity fault at `0x800B5244`.
One bounded retail run published MENU generation 4 and entered its native observer, then exhausted
the scoped original-call budget at `0x80018AA0` after 564,484 cycles.

Gap: Re-establish the complete reached sequence through pinned Lightrec with nonzero dynamic execution,
image-correct invalidation, bounded exits, and reason-accounted fallback. The next reached dependency
is preserving MENU entry's finite image-conversion work across a bounded original-call continuation
(issue 0029).

### S003 — Pinned-Lightrec gameplay executor

Partial capability: the direct player links pinned Lightrec as its first guest executor and reaches
BOOT execution. The asset-free product-link and selector checks exclude an interpreter-only default.
Representative gameplay, complete image publication, and a whole-run translated/fallback counter ledger
are still missing.

**MEASURED 2026-09-27, and the MENU image refusal is GONE — so the old wording above was stale.** The
consolidated resume owner (`cdc3ba5`) removed the abort. A 400-frame-capped headless run of
`build/player/bin/crashbash_port` now ends on the guest's OWN terms with a clean exit:

| measurement | value |
|---|---|
| aborts / `executor:error` / faults | **0** (previously an abort at the MENU image channel swap) |
| guest calls completed | **30,751**, of which **3 needed a resume**, deepest **7 host turns** against the cap of 12 |
| guest instructions executed | 32,521,896 in 2,839,510 executed blocks |
| renderer | `[gpu_vk] headless renderer up`, `present image 960x720 (headless sink)` — the Vulkan path DOES initialise |
| transform census | attempts=673 captured=673 **mismatch=0**; standard/alternate GTE input 673/673, 0 mismatched on rotation/translation/projection |
| BIOS card syscalls | 112 calls, **0 unhandled** |
| disc | 947 hunk lookups, 817 hits, 130 fills (13.7% miss) |
| last field | **f255**, then `[native_boot] frame loop done` |
| state reached | **one** — `0x8004E0B8` (entry `0x80010410`, update `0x80010394`, present `0x80010278`), and it is reported as `dwelling in state 0x8004E0B8 for 256 frame(s)` |

So the honest reading is narrow and worth stating precisely: **the product no longer aborts, executes
its whole guest loop cleanly, and its transform and syscall paths are clean — but it never leaves
`0x8004E0B8`, so "reaches f255" is NOT progress and gameplay is still unreachable.** Counting fields
without counting distinct states would have reported this as a large advance; the frame driver's own
`dwelling in state ... for N frame(s)` line is what makes the difference visible, and it should be the
number quoted for this title from now on.

Two instrument facts from the same run, both of which nearly produced a wrong claim:

- **`PSXPORT_NATIVE_FRAMES` is read through the legacy `cfg_int` path, not a declared CVar.** The boot
  audit reports it `UNKNOWN` because nothing has read it yet at boot, and the EXIT audit reports it
  `legacy (observed when read)`. Both are true; reading the boot line alone would have concluded the cap
  did nothing and that the run was unbounded, when the guest simply finished first.
- **`PSXPORT_VK_HEADLESS=1` is genuinely never read, and that is correct.**
  `runtime/psx/gpu_vk.cpp:178` is `s_headless = (cfg_on("PSXPORT_VK_WINDOW") && !cfg_on("PSXPORT_VK_HEADLESS")) ? 0 : 1;`
  — with no window requested the `&&` short-circuits and the override is never consulted. The product is
  headless because a window is the switch, which is the documented "a forgotten flag fails SAFE" design.
  The exit audit's `UNKNOWN ... NOTHING ever read it` is a true report of a no-op, and it is NOT evidence
  that the renderer failed to initialise. It briefly read as exactly that, which is worth recording
  because the renderer was up and presenting on the same run.

Still missing: representative gameplay.

**MEASURED 2026-10-01 — the whole-run ledger and the image-publication census EXIST, and the
execution numbers now belong to psxport.** `game/diagnostics/run_ledger.{h,cpp}` (library
`crashbash_run_ledger`) reports only what the framework has no counter for: WHICH authenticated
image was published, from which offer, with what witnesses, plus this title's refusals, guest
faults, native registrations and original-body calls. The dynarec's own numbers are printed by
psxport `runtime/cpu/execution_ledger.h` (`logRunEndLedger`, called from native_boot) in ONE format,
and this title deliberately prints none of them — a second copy in a second format is two answers to
one question. `tests/test_run_ledger.cpp` asserts the absence (`!printed("run-end executor:")` and
four more), so the duplication cannot come back unnoticed.

Measured on one 1,500-frame retail run against pinned psxport `d51ee05a`, log
`scratch/ledger/class-1500.log`, product `build/migration/bin/crashbash_port`, headless, silent,
unpaced, exit 0. **Framework `guest` channel:** 119,329 calls · 6,900 blocks translated / 14,670,173
executed · 214,993,195 guest instructions · 118,316 host dispatches · 14,663,272 cache hits / 6,904
misses · **25,993,839 invalidations, attributed**: cpu=21,518,393, mapped_store=4,473,896, dma=1,518,
module_load=4, debugger=0, savestate=0, native=28 · invalidation work 18,445,608 Lightrec calls of
which 18,442,941 answered by a guard, 2,667 walks over 97,890 block scans (7,680,960 before psxport `d51ee05a`), **57 blocks actually
revoked** · 27 budget exits, 27 with a PC inside a loaded code image, 0 outside · **0 interpreter
fallbacks**, with all five reasons and all five refused reasons present and zero.

**Title `crashbash-ledger` channel, same run:** 3 module offers / 3 published / 0 refused, plus 1
resident executable image, 4 distinct tracked images · resident generation 1 over
`[0x010000,0x079000)`, BOOT generation 3 over `[0x078C90,0x0D7490)`, MENU generation 4 over
`[0x0B32B4,0x0BB2B4)`, DAT28272 generation 5 over `[0x0B32B4,0x0C62B4)`, each with byte count,
content-witness count and the runtime's own invalidation delta across the publication · 28
overrides registered, 3 image generations bound, 98,914 original-body calls at 12 distinct sites ·
0 fallback refusal sites. DAT28136, DAT28382 and DAT22510 were **not** exercised: reaching them
needs the controlled mode selection issue 0032 still owns.

**`counters.invalidations` is REQUESTS, not invalidated blocks, and psxport now reports both
halves — so this title states no invalidation number at all.** `LightrecExecutor::invalidate` /
`invalidateAll` increment it once per CALL, and `Core::writeGuestMemory` issues one
`notifyExecutableWrite` per mapped guest store, which is why 25,993,839 requests (21.5M of them from
the CPU itself) stand against **57 blocks actually revoked** and 6,900 ever translated. The
`invalidation_work` and `invalidations_by_source` lines exist precisely because the request count
alone reads as four orders of magnitude too large; the first version of the title ledger printed
that request count under the words "reached the block cache" and had to be corrected.

**A run that ends by refusal still emits the ledger** — measured, not asserted. Pointing the product
at a byte-corrupted `SCUS_945.70` (log `scratch/ledger/measured-refusal.log`, exit 2) prints the
full ledger, names `run-end resident refusal: Crash Bash USA executable SHA-256 does not match the
authenticated manifest`, and says loudly that 0 executed instructions means the run measured NOTHING
rather than reporting a healthy set of zeros. `refuseRun` is the product's one refusal route for the
execution boundary and reports before it aborts, because `std::abort()` runs no destructor and the
framework's own `~LightrecExecutor` telemetry — documented as being emitted "on every exit path" —
is absent on exactly those paths (issue 0033).

**Two ledger requirements cannot be met from the title and are named as exact framework changes**
(issue 0033): an ADMITTED interpreter fallback leaves no port-visible site (the runtime keeps
`fallbackRefusalPc` private to its `Impl` and only sets it on the refusal branch), and "overrides
hit" has no framework counter at all (`NativeDispatcher::invoke` has no census; `hostDispatches`
includes BIOS/HLE). The ledger reports the refusal sites the runtime does surface and says which
half it cannot see, rather than publishing a number it cannot stand behind. psxport `5b66eb7f` has
since taken the other two thirds of this gap — the run-end guest ledger, and `invalidation_work`
with `invalidations_by_source`, which this title previously had to explain for itself.

**MEASURED 2026-09-30, and the "never leaves `0x8004E0B8`" reading above was a DEAD TAP.** The run
this paragraph described drove the pad at rest, so the guest ran the **attract cycle** — which is
what retail tells it to do until a button is pressed — and the frame driver was watching
`kAppModeVtable` (`0x8004E0DC`), whose one writer is `0x800101CC` and which holds the shell's own BOOT
scene for the whole run by construction. The mode machine is the scene record at `0x8009F658`; issue
0032 derives both facts from the authenticated bytes and `crashbash_scene_machine_test` censuses them.

One 4000-frame headless run with a real per-frame pad replay (Cross held 5 frames every 24 frames,
delivered through the guest's own SIO0 packet path, not a written button word), exit 0, pad replay
fully consumed 4000 of 4000 frames:

| measurement | value |
|---|---|
| live scene changes | **9**, every one the guest's own (logo → MENU `0x800B9524` → … → Crashball character select `0x8009F720`) |
| BOOT logo handoff | requested by the guest on `0x8001E588`: `scene 0x8009F658 -> 0x800A00DC request-flags=0x12` |
| MENU accept | `edge=00004000 current=800B8E28 pending=00000000->800B8E50` — the guest's own Cross edge |
| controlled mode reached | **DAT28136** published (generation 5, 42 sectors, LBA 28136), then its registration replaced app callback `0x80093038` with `0x800B4694` |
| guest calls completed / resumed | 851,945 / **6**, deepest **7** host turns against the derived cap of 12 |
| executor calls / blocks / instructions | 873,988 / 53,768,141 / 840,634,226 |
| fallback blocks / instructions | **0 / 0**, every reason 0, every refusal 0 |
| faults | 0 |
| presented frames | menu at f100; Crashball character select at f700/1600/3900, **689,194/691,200 non-black (99.71%)** each |

So the S003 ledger now exists for one representative run that leaves the attract cycle, on the
dynarec-first product, with zero fallback. It is still **partial**: one run does not cover every
resident and nested module (the idle attract loop publishes DAT28272 and DAT28241, the Cross route
publishes DAT28136, and none of the five has been seen in one run), and the residency set S003 names
still has to be proven complete per module.

**The live-match leg, 2026-09-30 — one process, 3,740 frames, `replays/flow/crashball-control.pad`,
exit 0.** 824,429 executor calls, 56,755,797 executed blocks, 881,471,499 executed instructions;
`fallback_blocks=0`, `fallback_instructions=0`, every reason counter 0, `refused_fallback_blocks=0`,
`max_fallback_blocks_per_execution=1`; 803,814 guest calls completed, 9 needed a resume, deepest 7
host turns against the derived cap of 12, 14,820,784 guest cycles over those calls; the replay was
consumed 3,740 of 3,740 frames. The route leaves the attract cycle, publishes **MENU** and
**DAT28136**, then publishes **DAT28241** at frame ~2000, which is the module that runs the match:
at the shared `0x800B32B4` slot the *same* address `0x800B4694` holds DAT28136's registration
callback in one image generation and DAT28241's code in the next, which is the loaded-image identity
case S015 measures rather than a scene change. Presented output at the match frames is
553,189-557,800 of 691,200 non-black (80.03-80.70%).

### S004 — Native graphics coverage

Evidence: Native model, sprite, authored-screen, ordering, camera, and scene-snapshot owners render the
measured startup, menu, Crashball, Crate Crush, Tournament, and Polar Push content from decoded game
state rather than GTE/OT/GP0/framebuffer output.

Gap: Requalify these owners across representative dynarec gameplay and complete the remaining visible
4:3, wider-mode, effect, UI, and transition coverage.

### S005 — Widescreen camera

Evidence: The native camera shows additional horizontal Crashball coverage while preserving vertical
framing, and authored 4:3 presentation compositions remain centered. The owner widens the projection
host-side (`projection.ofx += margin << 16`, with the authored-screen draw area clamped to the centred
viewport) and never writes a guest projection scalar.

**The rule's home is the S005 row above**, which already states the sourcing policy (margin objects from
object memory, margin-only objects animated port-side, guest memory untouched). Two points this section
adds that the row does not carry, and one it withdraws:

- **The reportable gap is a margin object ABSENT from object memory** — despawned by a camera window
  rather than culled from a list. A culled-but-resident object is the rule working as intended, not a
  finding.
- **Withdrawn as a gate:** "measure the widescreen gameplay-read hazard (camera `+0x18` behind
  `*0x800569E0`)" as a *precondition* for widening. No guest window is widened — not the camera
  window, not `H`, not a draw-area or scissor word — so there is no guest scalar to be
  gameplay-unsafe. The camera-struct observation is still a worthwhile fact about the title; it no
  longer gates the feature.

**Measured, 2026-09-30 — the RAM identity invariant, both legs, one replay, 3,000 frames.** The
`aspect` knob is the settings file (`PSXPORT_SETTINGS`, `aspect=0` = 4:3, `aspect=1` = 16:9), not an
environment variable. The two legs are `native_width=512 render_width=512` and
`native_width=512 render_width=684`; the aspects are confirmed distinct rather than one silently
falling back to the other.

| check | result |
|---|---|
| guest RAM across aspects, frame 2500 | **524,288 / 524,288 words identical (100.0000%)** |
| guest RAM across aspects, frame 2800 | **524,288 / 524,288 words identical (100.0000%)** |
| object-memory population | **identical at 12 of 12 census frames** in every category — draws, transformed, source-decoded, faces captured, textured all equal. Only `faces submitted` differs, by at most **9 faces** (1218/1216, 1720/1718, 2799/2808, 2798/2807 — under 0.4%) and **in both directions**: a projection change moving a few faces across the clip boundary, not a cull removing objects |
| capture corroboration, read off the two PNGs rather than a logged counter | both `present_3000.png` captures show the same HUD score digits 14 / 15 / 14 / 15 and the same ball position; the 16:9 frame carries additional arena at both edges |
| reportable gap class — a margin object **absent from object memory** | **zero objects**, from 524,288 scanned words × 2 aspects × 2 frames. This follows rather than merely agreeing: object memory *is* guest memory, the guest bytes are identical across aspects, so an object present in the 4:3 population and missing in the 16:9 one would require a guest write that did not happen |

Method note, because the tool refused rather than reporting a vacuous result: the first comparison
asked for a frame-3000 dump that neither leg has — `PSXPORT_NATIVE_FRAMES=3000` ends before it — and
the audit exited 2 with `REFUSED` instead of printing "2 of 3 frames identical". The two frames both
legs do hold are the two rows above. The census is a **count** comparison, and the RAM identity is
what carries the set-identity claim; the census corroborates it and is not the proof.

Gap: frames 2048-3000 are not covered by the per-frame census schedule (it reports on a geometric
dwell schedule, last at frame 2047), so the identity result is measured at frames 2500 and 2800 and
corroborated at 12 census frames reaching frame 2047. The other representative retail modes (Battle,
Tournament, Polar Push) have not been surveyed under this rule.

### S006 — Interpolated presentation

Evidence: The native path rebuilds midpoint model geometry from consecutive immutable title snapshots
without changing the retail simulation cadence or mutating guest RAM.

**THE RETAIL CADENCE IS NOW MEASURED, and it is 2 FIELDS PER GAME FRAME = 30 fields/s** (docs/issues/0031).
The number was absent from this repository, so nothing here could say whether interpolated presentation was
the right thing for this title at all. It is: the title runs 30 fps, so it is in lerp scope, and preserving
the retail cadence is what the native path already does. From the provisioned image, not the listing — four
of five writers store the literal 2 into the rate global `0x8004E0E0` (`guest::kDisplayFieldsPerFrame`), and
the display owner passes it straight through as the VSync argument. `0x800320EC` proves the argument is a
FIELD COUNT: `a0 == 1` and `a0 <= 0` both take a **no-wait** branch and only `a0 >= 2` waits, for
`current + a0 - 1` more fields. **60 fps is therefore structurally unpaceable on this path**, because
`VSync(1)` is the explicit no-wait case; a skip-every-other-VBlank shape would need a counter compare and the
process loop has none. Corroborated independently by the process runner looping `{update; present}` with no
counter, parity test or comparison, and by exactly one of 54 VSync call sites having a non-literal argument.

**Denominator stated honestly:** the census is every `imm16 == 0xE0E0` paired with `lui $reg, 0x8005` over
**323,072 words across all 8 linked images**, giving 7 sites — 5 writers, 2 readers — with **0 in the six
gameplay/MENU overlays**. **The fifth writer is the open item:** BOOT `0x80094694` stores **3**, not 2, from
`$a2 = 3` in a delay slot, reached only via a `beqz`, inside a fixed-point divide. Whether that path is
per-frame or one-time init is not established. It does not change the gameplay answer, because the overlays
never touch the global and the resident image re-asserts 2 at every scene start — but "four writers say 2,
one says 3" is not "the cadence is 2", so the denominator is five writers.

Gap: Complete and verify interpolation for all reached world, effect, UI, and transition families on
representative hybrid gameplay.

### S007 — Independent diagnostics

Evidence: Existing emulator comparisons and title-local judges cover interrupt ordering, module loads,
command phases, input delivery, menu transition, native ownership, and graphics attribution with
controlled opposite answers.

Gap: Migrate every still-useful judge to the shipping native/dynarec boundary and add product-link,
selector, translated-block, override/original-call, invalidation, and representative-gameplay coverage.

### S008 — End-to-end retail mode coverage

Evidence: Durable bounded scenarios cover live, controllable Crashball, Battle Crate Crush, Tournament
Crate Crush, and Polar Push behavior.

Gap: Re-run that frontier through pinned Lightrec and cover every remaining retail mode family.

### S009 — Crashball scenario

Evidence: The tracked 3,740-frame replay reaches a live DAT28241 Crashball match and visibly moves the
player ship left and right.

### S010 — Battle Crate Crush scenario

Evidence: The tracked 15,401-frame replay crosses the objective, controls, and special-items pages,
enters a live Battle Mode Crate Crush match, and moves the player in both directions.

### S011 — Tournament Crate Crush scenario

Evidence: The tracked 7,910-frame replay reaches Tournament Mode's first live Crate Crush match and
moves the player in both directions.

### S012 — Polar Push scenario

Evidence: The tracked 17,682-frame replay reaches a complete Polar Push match with the four-player HUD,
arena, ships, balls, and controllable player movement.

### S013 — Remaining modes

Missing capability: Add durable, controllable hybrid-product scenarios for every retail mode not
covered by the four retained gameplay routes.

### S014 — Audio playback

Evidence: The measured host sink sustained 44,097 samples/s against the 44,100 Hz target with zero
dropped fields, while the bounded pre/post WAV remained byte-identical.

Gap: Requalify hardware listening, longer gameplay, and every title audio path through pinned-Lightrec
gameplay.

### S015 — Runtime overrides and original calls

Evidence: the surviving title sources contain 28 registrations through the single image-qualified
`runtime::registerNativeOverride` boundary and all 16 former generated-body calls now enter the single
scoped `runtime::callOriginal` boundary. `tools/verify_native_ownership.py` reports both denominators
and its test suite proves forbidden old paths are detected.

Implementation: `game/core/guest_execution.{h,cpp}` now supplies the per-Core adapter. Native owners
can register before their module is resident. The authenticated loader supplies the logical image,
shared catalog identity/generation, and complete physical range; registration publication and original
calls reject a mismatched residency. Original calls use shared scoped suppression, and replacement or
unbind removes the old generation's native keys. The context owns no independent guest image catalog.

Focused evidence (2026-09-08): `ctest --test-dir build/migration -R '^crashbash_guest_execution$'
passed 3 cases / 28 assertions against psxport `bf833b54`, configured with Clang and the frozen uv
interpreter. The shipping dispatch/original wrappers executed 2 Lightrec blocks / 6 instructions with
zero fallback. Overlay replacement and wrong-image original calls refused, an invalid replacement
preserved its prior binding, two Core contexts stayed isolated, and changing the guest body changed
the native-plus-original result from 17 to 19. Both touched translation units passed clang-tidy and
format checks; source policy retained the 27-registration / 15-original-call denominators.

The direct `TitleAdapter` now composes the per-Core execution context, all native owners, BIOS memory-card
device publication, native frame driver, and immutable-scene interpolation presenter. The new
`game/core/player_entry.cpp` composes that adapter with the heap-owned `Game`, authenticates the
resident executable, binds per-Core hardware owners, and enters `native_boot_run`. Its resident loader
hashes the entire executable against metadata derived from `titles/crashbash/executable.json`, loads that
same byte span through shared `loadPsxExeImage`, and binds the returned generation. Failed authentication
preserves guest state; successful replacement retires the prior resident generation. The retained
instruction accounting, deferred-work polling, and break handling now call shared execution services.
The complete retained seam and `crashbash_title_adapter_test` now compile and link with Clang/Ninja;
an unchanged second build performs zero compilations. Focused clang-tidy passed all 11 touched/new
translation units, formatting passed 17 C++ files, and source policy reported 84 sources with the
27-registration / 15-original-call denominators intact. The freshly linked `crashbash_title_adapter`
CTest passed 3 cases / 28 assertions. Its explicit local real-input run against `SCUS_945.70`, using
`scratch/title-adapter-test/card.mcr`, passed 4 cases / 43 assertions: complete retail authentication,
resident native-key publication, corruption refusal preserving the prior residency, and valid reload
leaving exactly one active image generation. These runs load the resident image without executing
retail boot or gameplay.

Landing verification (2026-09-08): the canonical verifier configured Clang/Ninja against psxport
`156c6c58`. The full build exposed module tests that omitted the execution adapter; one reusable
`crashbash_guest_execution` library now owns it for the seam and those tests, and the full build
passes. All 25 functional/Python/pin CTests passed. The remaining C++ policy check initially refused
stale documentation and literal rejection data; focused repairs preserve the binary findings and all
negative checks. Its completed components pass: 91 formatted/size-checked files, 53 compile-backed
translation units linted, and whole-tree architecture/execution policy. The shared rejection-data
scanner passes 34/34 discriminator cases. The title source policy passes 7/7 tests and reports 85
sources, 27 registrations, and 15 original calls. The linked adapter passes the shared execution-boundary
check and its checker selftest. After the scanner correction landed in psxport `a5a79652`, canonical
configure and the dependency check aligned the pin/provenance without recompilation. The formerly
failing `crashbash_cpp_policy` CTest then passed on two CPUs, completing all 26 title CTests across
the original run and focused repair. No additional retail gameplay was run.

Focused player-entry evidence (2026-09-12): `crashbash_port` links with Clang/Ninja and the focused
adapter, execution, touch, scene, and render-capability tests pass 5/5. A real run with the user
SCUS_945.70 executable authenticates the resident image, publishes the title owners, initializes the
guest heap, enters the finite boot prefix, opens the real CHD, and then fails fast at
`0x80012E90 -> 0x800279A4 -> 0x80034AFC -> 0x80034B8C -> 0x8003F29C -> 0x8003EBF8 -> 0x8003E6B0 ->
0x800320EC`: the strict VSync service requests `FrameBoundary` after 128 cycles while the startup
call requires `GuestReturn`. This proves the executable entry and authentication route and identifies
the next runtime owner; it does not prove boot, gameplay, or presentation.

Framework alignment (2026-09-12): the consumer build ran against shared psxport `8b210329`. The
combined Clang gate passed all 28 title CTests, including the then-live pin check and the shared
execution-boundary check. (The pin mechanism was retired 2026-10-02; `external/psxport` is now the
workspace's live checkout.)

Direct CD migration evidence (2026-09-12): The old `GameConfig` bound stock CdCommand, CdSync, and
CdSearchFile, but the direct plan initially declared only VSync. The framework now exposes those
three standard CD services as typed plan fields and Crash Bash binds the measured entries. Focused
Clang tests pass 3 cases / 13 assertions for direct and legacy CD registration, 6 cases / 45
assertions for stock CD command/sync behavior, and 4 cases / 49 assertions for title composition and
retail executable authentication. In a bounded headless direct player run, the USA CHD opened,
`load file start` and `done loading` completed, and the native frame loop began with no guest VSync
trap. That run stopped at an unknown BOOT image at `0x80092BDC` after 153,924 guest cycles. BOOT
publication and partial-overwrite retirement now cross that boundary. A later bounded run published
MENU from its exact measured read and entered `0x800B5244`; its original call then exhausted the
current-turn budget at `0x80018AA0` after 564,484 cycles. This advances the startup frontier but
does not qualify a presented frame or gameplay, and the abort prevented fallback-denominator output.

Gap: resolve the reached MENU original-call budget boundary (closed by issue 0029), then qualify all 28
installations and 16 original calls on real loaded-image and gameplay routes. The shared
direct-runtime memory-card path also needs OS user-data configuration; its current scratch fallback
is not a releasable save location.

### S016 — Representative gameplay

Partial capability, advanced 2026-09-30 from the character-select stage to an **interactive live
Crashball match** on the dynarec-first product (issue 0032).

**How the match is reached, from the guest's own code and nothing else.** The tracked
`replays/flow/crashball-control.pad` drives it. The input shape is not guessable and the difference
matters: Cross taps every 32 frames through frame 1192 leave the attract cycle and reach the menu, but
**tapping Cross alone reaches character select and then sits there indefinitely** — on that stage
Cross cycles the roster instead of confirming. The route needs the 800-frame Circle hold at frames
1200-1999; Cross at 2000 publishes DAT28241 and starts the match, Cross at 2560 leaves the briefing.
Measured scene changes on this run: logo `0x800A00DC` → MENU `0x800B9524` → `0x8009F720`
(enter `0x80092CAC`, update `0x80092EDC`, present `0x80092E94`) → `0x800A0BF4` → `0x8009E5C8` →
`0x800A00DC` → `0x8009F720` → … 10 live changes, every one taken by the guest through the scene
machine. The observer is read-only: it calls the retail body at `0x8001E588` and reports the record
before and after.

**Player movement, proven against a control leg rather than asserted.** Two runs of the *same*
3,740-frame replay, differing only in 180 frames of direction input:

- **treatment** — `replays/flow/crashball-control.pad`, Left held 3560-3619, Right held 3620-3739;
- **control** — the byte-identical replay with every frame from 3560 on forced to `0xFFFF`
  (180 frames changed, every other frame byte-for-byte equal), so the control receives no direction
  at all.

Guest RAM was dumped whole (2 MiB) at frames 3555, 3575, 3595, 3615, 3625, 3650, 3700 and 3735 in
both runs. Measured, with denominators:

| claim | measurement |
|---|---|
| the input really reaches the guest | `0x80063A92` (parsed P1, active-low) `0xFFFF` → `0xFF7F` under Left → `0xFFDF` under Right; `0x8005133C` (active-high P1) `0x0000` → `0x0080` → `0x0020`. In the control leg **both stay at rest across all 8 frames** |
| the two legs are the same run | frame 3555, which precedes the first held direction, differs in **0 of 524,288** words; from 3575 on the legs diverge (6,039 / 8,558 / 11,651 / 15,794 / 18,565 / 25,906 / 29,354 words), so the dump is deterministic and the later differences are caused by the input |
| guest words move in the commanded direction | of 524,288 scanned words, **159** decrease monotonically across all three Left steps *and* increase monotonically across all three Right steps. A word that merely drifts will not satisfy a sign flip on the commanded axis, so this is a filter, not a coincidence count |
| the movement is caused by the input | every one of those 159 words also differs from the idle control leg at 6 of the 7 post-input frames. Representative, all at the same frame set: `0x8005721C` (a small signed screen-space value) `118, 113, 101, 99` under Left then `95, 98, 99, 116` under Right, against a control that never leaves its own trajectory; `0x80056ACC`/`0x80056ADC` and `0x801D4048`/`0x801D40FC` move the same way, the last pair crossing zero as the commanded direction reverses |
| the match is live, not a still | the ball is in play with a motion trail, and the HUD score digits advance during the window (P1 `12` at frame 3560 → `11` at 3640 → `09` at 3730, as the ball reaches the left goal; the other three digits hold) |

**Citation status, stated precisely.** `0x8005133C` has 10 `lui`+displacement sites across the
authenticated images and is fully attributable; `0x80063A92` has **0**, because the guest reaches it
through a base register no `lui` materialises. The movement words are reached the same way:
`tools/probe_addr_refs.py --stores-only` returns 0 sites for `0x80056ACC`, `0x80056ADC`,
`0x8005721C`, `0x801D4048` and `0x801D40FC`, and no 32-bit word in the dumped RAM equals any of them
or a plausible base for them, so the effective address is base+index computed in a register. The one
base in that band the images do materialise by `lui` is `0x80056998`, at BOOT `0x800825B4` and
`0x80082624` (`lui $v0, 0x8005; addiu $s0, $v0, 0x6998`) inside the input-edge handler that tests the
direction bits `0x4000` and `0x40`. **The individual store instruction is therefore not yet
attributed, and this row does not claim it is.** Closing it needs the base+index loop resolved, which
is the next RE step (issue 0032).

**Per-frame host time, 3,740 frames, pacing disabled** (`PSXPORT_DEBUG=perf`, `PSXPORT_NOPACE=1`).
Whole-run distribution: **p50 9.50 ms, p95 13.25 ms, p99 18.50 ms, worst 359.77 ms, 3 frames beyond
the histogram range**. Inside the match and movement window the 60-frame averages are 10.16 ms
(f3480), 11.29 ms (f3540), 10.78 ms (f3600) and 11.32 ms (f3660). Two facts about what this does and
does not say: the 359.77 ms worst case is a one-off module instantiation, not a per-frame cost, and
these are **unpaced host CPU costs** — the title still runs two display fields per game frame, so
9.50 ms p50 is not a claim of 105 fps gameplay. The `audio` phase slot reads 0.00 **by construction**:
the per-field SPU advance is inside the `game-logic` span because `GpuPerf`'s phases are a partition
and a nested bracket would double-charge the same wall time and drive the reported idle negative.

**A defect this milestone found in the instrument, not the product.** Nothing in the title ever called
`Game::perf.frameBegin()/frameEnd()`, so the framework's per-frame profiler — enabled, healthy, and
covered by a passing framework test — produced **no timing line at all** for this title. That reads
exactly like a clean measurement of absence, which is the failure mode this workspace has hit nine
times. It is now bracketed in `CrashBashFrameDriver::stepFrame`. Related and also corrected: the
channel is selected by **`PSXPORT_DEBUG`**, not `LUCENT_DEBUG` (`cmake/psxport.cmake` sets
`LUCENT_CHANNEL_ENV=PSXPORT_DEBUG`); an earlier leg of this work set the wrong variable and the
channels it appeared to enable were emitting only because they are info-level.

Still missing for `verified`: audio/timing coverage from the headless WAV sink, and per-frame
frame-time evidence on each other released host architecture. Battle, Tournament and Polar Push
remain un-requalified on the dynarec product.

### S017 — Break-first static-path removal

Evidence: the tracked offline emitter integration, seed file, generated registry installer, and
static-only verifiers are deleted. The ignored generated corpus, prior static build tree, and retained
static product binaries are absent. `tools/source_policy.py` rejects the old files, generated directory,
static dispatch markers, and any change to the 28-registration/16-original-call source boundary.

### S018 — Platform CI coverage

Partial capability: `.github/workflows/ci.yml` configures a Linux x86-64 native adapter job with full
history, read-only permissions, pinned actions, and an explicit timeout. It resolves the framework with
`tools/psxport_fetch.py` (a clone of psxport `main` in CI, which has no sibling checkout), uses that
checkout's shared Linux setup action, and runs `tools/verify.py`. The thin title verifier
selects the real `crashbash_title_adapter_test` artifact and every `crashbash_` CTest; shared
`ConsumerVerifier` owns build, style/test execution, and linked execution-boundary checks. Hosted
execution of this expanded job remains unverified, and it does not establish packaged gameplay.

| Platform | Applicability | Current CI evidence and exact gap |
| --- | --- | --- |
| Linux x86-64 | applicable desktop target | Native adapter build, complete title tests/style, and execution-boundary inspection are configured through the shared verifier; hosted result and packaged gameplay remain unverified. |
| Windows x86-64 | applicable portable-PC target | Missing: no supported Windows native build, runtime test, first-run setup, or package boundary exists. |
| macOS arm64 | applicable portable-PC target | Missing: no Apple-Silicon native build, runtime test, first-run setup, or application package exists. |
| Android arm64 | applicable mobile target | Android metadata and setup sources exist, but the Lightrec native runtime, shared `android-port` build path, Gradle/NDK APK build, and install/runtime test are missing. |

Gap: the former green Android metadata/selftest job was removed because it did not build or install an
APK. Add Android and desktop jobs only when they drive the actual redistributable platform boundary
without game assets.

### S020 — Crash Bash loading removal

Verified. The loading-only wait is gone, and the guest's own LOADING screen no longer reaches the
display: it is the card draw the handoff's two screen presents make, retired at those call sites.
The transition clock the handoff runs on is authored and is kept.

**Where the wait is.** Retail's load pump `FUN_8001231C` advances the queue exactly one step per call
and returns 0: tick the `DAT_80050628 = 3` inter-read cooldown, poll-complete the active read at
`0x8005062C`, or start the next queued read at `0x80050634`. Eight sites call it (measured with the
decomp pipeline: `0x8001039C`, `0x800132EC`, `0x80013384`, `0x80013454`, `0x800134C8`, `0x80013538`,
`0x80016CF8`, `0x8001E724`). Seven are load-completion loops — `while (iVar2 = FUN_80012FFC(), iVar2 !=
0) { FUN_8001231c(); FUN_80010ae8(&DAT_8004e0f0); FUN_8002bae8(); }` in `FUN_8001e610` and its five
siblings — which spend no frames. `0x8001039C`, the main per-frame call, is the entire wait: three
cooldown frames per queued read, on top of a disc read that completes synchronously here
(`cdFileReadOwned`). The `kLoadPump` override therefore runs retail's pump to quiescence at that one
site and gives every other caller exactly retail's single step, so each loop keeps driving its own
queue with its own helpers.

**The two helpers stay with the loop callers.** `0x80010AE8(&DAT_8004e0f0)` drains the frame-heap
work a read completion enqueues (`0x800110a8`) and `0x8002BAE8` publishes the pending-completion
record at `0x800654C0/0x800654C4`; retail calls both only inside those seven loops. Calling them from
the per-frame site is not a shortcut — measured 2026-10-02, a drain that also emptied the queue from
the LOOP callers left the Polar Push briefing refusing Cross with no read ever requested, because the
loop body, and with it the helpers, never ran.

**Evidence, one keyed route (`replays/flow/polar-push-control.pad`) on the pre-S020 and S020 builds.**
Field counts from each run's own scene-change lines, at the two display fields per frame the driver
reports: boot/logo load scene → menu world `f460→f512` = 52 frames = **104 fields** on the pre-S020
build against `f460→f475` = 15 frames = **30 fields** on this one; briefing → match load scene →
match `f1038→f1113` = 75 frames = **150 fields** against `f1001→f1022` = 21 frames = **42 fields**
(`scratch/s020/ev-head.log`, `scratch/s020/ev-drain.log`). Both runs authenticate
`crashbash-usa-dat22510` (71 sectors, the Polar Push module) and both move the P1 record word under a
held Left (`ev-head` replay 0xFFFFFBC0→0xFFFFF5C9, `ev-drain` replay 0xFFFFFB61→0xFFFFF6EC), so the
route, the payload and the control are the same on both sides of the change.

**What the LOADING screen turned out to be.** The card is not a scene of its own and the handoff is
not held by a counter: `0x800A00DC` IS the game's transition scene (every scene change in the route
passes through it), and the card is drawn by the resident `FUN_80018B08`, which BOOT reaches only
from the two handoff screen presents — `FUN_8009421C` (screen `0x8009F998`) at call site
`0x80094248`, and `FUN_8009414C` (screen `0x8009FA00`) at `0x80094188`. Both presents are entered
only while `0x800A00DC` is current, so those two sites are the whole of the card's presentation.
Measured 2026-10-03: the handoff's own work is done by f468 (state 4→7→9, `DAT_800A0158`), and the
frames after it are the scene transition clock `FUN_8001E598` on `0x8009F644` — −0x300 from `0x700`
to 0, then +0x300 to `0x1000` until `FUN_8001E610`'s age gate admits the destination. That clock is
the authored fade for EVERY transition in the game (the world scene's own enter `0x80092CAC` arms
−0x300 again), so it stays; what shipped was the card, drawn on top of it. `game/core/loading_card_skip.{h,cpp}`
now retires the card at those two measured call sites and runs every other caller of `FUN_80018B08` —
the menu screens draw their own panels through it — on the guest's own body. The handoff therefore
presents the level the guest has built, and the authored fade carries it to black and into the next
real picture.

**Evidence, the same keyed route.** Field counts are unchanged from the drain build — boot→menu
`f460→f475` = 15 frames = **30 fields**, briefing→match `f1001→f1022` = 21 frames = **42 fields** —
because the clock is authored and was not touched; what changed is that none of those fields shows
the card (`scratch/s020d/base_sheet.png` against `fix_sheet.png`: LOADING at f466/f468/f472/f474
before, the studio logo held at f462 then the level's own backdrop at f466/f470 after). The dense
captures are `scratch/s020d/handoff1_sheet.png` (26 consecutive presented frames over boot→menu: logo,
held studio screen, the level's own backdrop, menu fading in) and `handoff2_sheet.png` (28 frames over
briefing→match: held briefing page, the authored fade through black, the match fading in). Neither
contains a card.. The run still
reports replay COMPLETE 1394 of 1394 and reaches the live Polar Push match
(`scratch/s020d/fix_match.png`, timer 1:24), where the P1 record word `0x800AD304` holds −2179 while
idle and moves to −2694 under a held Left.

**Phase-keyed replays.** `replays/flow/polar-push-control.pad` is **phase-keyed v1** (1,394 frames, 13
segments, keyed through `crashbash::InputPhase` on scene `0x8009F658` packed with the menu screen at
`0x8009F8A4`, unit-tested in `tests/test_input_phase.cpp`). It was recorded by tapping the menus —
one 4-frame tap per screen, chosen from the two scene records, and one Cross per briefing page — on
this build, with the live match recognised by the P1 record word answering a held Left, because the
briefing and the running match share a phase (both measure outer `0x8009F720`, menu `0x00000000`).
Replayed from the file alone it completes all 13 segments with 1,394 of 1,394 frames delivered on
BOTH builds, reaches `dat22510`, and takes control. The former absolute 4,547-frame file reached the
same match on the pre-S020 build but left the route on this one, entering the LOAD GAME dialog
(`0x8009F480`, "THERE IS NO CRASH BASH DATA ON THIS MEMORY CARD") once the collapsed handoffs moved
its presses off the screens they were recorded against; the phase key is what makes the file survive
the change. `crashball-control.pad` is still absolute from boot and was checked against S020 rather
than assumed: it completes 3,740 of 3,740 frames and reaches a live Crashball match with all four
scores running (`scratch/s020/cb-drain.log`, `cb-drain_replay_end.png`). The other three are not yet
measured against S020.

## Dynamic migration acceptance

The first Crash Bash dynamic milestone must prove all of the following together:

- the exact authenticated resident and loaded images execute through the pinned psxport/Lightrec
  integration with nonzero translated-block execution;
- product link and configuration inspection excludes an interpreter-only default, with every fallback
  reached only after an explicit JIT rejection and bounded by reason-accounted counters;
- all 30 native override registrations, across the 18 installation entries in the owner set, are keyed by
  complete runtime image identity and address;
- all 16 former generated-body calls use a scoped original call that suppresses only the current
  override, enters the original guest body through Lightrec, and returns with correct guest state;
- loaded-image replacement at the shared `0x800B32B4` slot invalidates affected translated blocks and
  cannot reuse an override or block under the wrong image identity;
- bounded executor exits preserve VSync/frame, interrupt, exception, host-work, and thread-exit
  ownership without unwinding through translated host frames;
- the existing independently controlled boot, Cross-menu, Crashball, Battle Crate Crush, Tournament
  Crate Crush, and Polar Push scenarios reach their recorded boundaries without a guest-VSync
  violation, wrong-image dispatch, or missing native owner.

Boot, logos, menus, and a first frame are checkpoints only. S016 requires representative interactive
gameplay with observable movement, correct rendering, audio/timing coverage, and declared frame-time
evidence on each released host architecture. The static execution path is already absent and cannot be
used to obtain new evidence; comparison comes from the independent emulator, binary analysis, or a
separately built test target.

## Retained verified title facts

These facts survive the execution-engine migration because they describe the retail binary, user-visible
behavior, or native subsystem contracts rather than the retired translation method.

### Authenticated images and overlay identity

- USA `SYSTEM.CNF` boots `SCUS_945.70`. The PS-X EXE has SHA-256
  `fd5727a18feb2a2d5a6359a55966f0266284d1e50f64ee9b8a127a97091bd516`, entry
  `0x8002E7B0`, load address `0x80010000`, and text size `0x69000`.
- Real CDC/DMA traces place BOOT at LBA 35799. MENU, DAT28272, DAT28241, DAT28136, DAT28382, and
  DAT22510 are authenticated alternatives in the reused nested load region beginning at
  `0x800B32B4`; their tracked manifests remain the identity authority.
- Polar Push DAT22510 is `CRASHBSH.DAT + 0x02B81000`, 71 sectors, size `0x23800`, SHA-256
  `7477dd74ccd80f8f1b8ce63265f8acf130e36a97f1e9182767383556e8ffc7e1`, loaded at
  `0x800B32B4`. Runtime image generation must distinguish it from every other occupant.
- DAT28382 installs initializer `0x800BB370`. DAT22510 uses callback `0x800CBA64`. The measured
  Cross path registers DAT28136 at `0x800B4E1C`, replaces application callback `0x80093038` with
  `0x800B4694`, and then executes that update.
- DAT28136 is the 42-sector image at `CRASHBSH.DAT + 0x0367E000`. DAT28241 is the 31-sector image
  from LBA 28241. DAT28272 is the 38-sector image from LBA 28272; its code registers a behavior vtable
  at `0x8005AA70`, while its name table identifies animation states rather than a second menu phase.

### Boot, device, and frame ownership

- ResetCallback `0x80031A80` calls setjmp `0x8003ACEC` and resumes at `0x80031AE8`; the independent
  oracle observes `0x80031AE8` (`v0=1`, `sp=0x80068B14`) to `0x80031B58` with the same stack and
  `ra=0x80031AF8`.
- The finite native boot owner begins from the measured one-shot `0x8002718C` / `0x80010158` prefix;
  repeating process work remains owned by the frame driver.
- The IRQ2 callback is `0x8003F5F0`, draining through `0x8003E14C`. The VBlank/SIO chain registers
  class 2 element `0x8006D984`; verifier `0x8003B1BC` tests I_STAT bit 0 and handler `0x8003B224`
  drives SIO0, per-byte I_STAT bit 7, and timer 2.
- Measured native ownership boundaries include memory-card startup `0x800486DC`, libcd command/sync
  `0x8003EBF8 -> 0x8003E6B0`, TOC readiness `0x800349AC -> 0x8003584C`, file-read start
  `0x80027790 -> 0x8003470C`, controller handshake `0x80034B8C`, and disc/license state machine
  `0x8002D4F4`. BOOT object callbacks `0x8008ADA4` and `0x8008BB48` are native frame owners.
- The card-event callback is `0x8004718C`; retail code constructs its address at `0x80047280` rather
  than storing a discoverable pointer. The libmcrd device-table walk at `0x8004799C` returns zero for
  both “no such device” and “request started,” so publishing the `bu` device and its completion event
  are independently required.
- The disc owner performs real CHD reads and may complete them synchronously, but it must not fake
  completion, manufacture a guest VSync clock, or weaken frame/watchdog ownership. The independent
  asynchronous oracle remains diagnostic evidence for the 189-sector completion sequence.

### Input and representative flow scenarios

- The pad chain changes packet `0x80077FBC` from `41 5A FF FF` to `41 5A F7 FF`, parsed P1
  `0x80063A92` from `FFFF` to `FFF7`, and active-high P1 `0x8005133C` from 0 to 8; P2
  `0x80051394` remains 0. Direct-buffer injection is not an accepted path.
- The active MENU table `0x800B8E28` updates through `0x800B3CA8`. Rising-edge input
  `0x80051380` accepts Cross `0x4000` at `0x800B3D88-0x800B3D8C` and schedules table
  `0x800B8E50` through `0x8009F8A8`. START is not a substitute for this transition.
- Portal-selection byte `0x8005A677` changes from `0xFF` to `0x00` before the DAT28241 load. Guest
  state `0x8004E0B8` and application mode `0x80078C90` remain the measured transition witnesses.
- `replays/flow/crashball-control.pad` contains 3,740 active-low masks; frames 3560-3619 hold Left
  and 3620-3739 hold Right in a live DAT28241 Crashball match.
- The recorded 15,401-frame Battle Mode replay reaches a live Crate Crush match; the 7,910-frame
  Tournament replay reaches its first Crate Crush match; the 17,682-frame Polar Push replay reaches
  a complete, controllable live match. These are the minimum retained scenarios for dynarec
  requalification, not claims that every retail mode is covered.

### Native graphics and presentation contracts

- Runtime projection anchors are `0x800193A8` and `0x8001AF2C`; model/object ancestry includes
  `0x80019A60`, submitters `0x80019F1C`/`0x8001DD50`, transform composition `0x8001965C`,
  source decode `0x8001C1E0`/`0x8001C0F0`, and prefill `0x80017EE8`.
- The stable resident ancestry is `0x80015780 -> 0x8001CD04 -> {0x800193A8, 0x8001AF2C}`;
  runtime callers `0x80019DAC` and `0x80019D7C` independently reached the two projection anchors.
- `0x8009440C` proves projection H comes from `camera + 0x18`; `projectionGlobals + 4` is a separate
  horizontal OFX scale. `0x80018B08` is viewport/camera/render-list setup, not a drawable boundary.
- Native sprite boundaries `0x8002992C`, `0x80029D28`, and the authored-screen branch of
  `0x8001A0D8` consume game-owned descriptor, position, color, texture, scale, fade, and ordering
  data. They do not consume OT, GP0, VRAM, or framebuffer output as product inputs.
- `0x800274FC` and `0x800276C4` are the two heap-pool allocators. Their live base/end ranges, the
  4,096-entry ordering tables, frame-wide authored ordering, AVSZ3/OTZ rejection, and immutable
  scene snapshots remain native contracts.
- Retail initialization at `0x80033494` publishes ZSF3 341. Packet `0x800C5394` identifies object
  `0x800A0C74`, frame `0x200B`, face 261, material `0x02FB`; AVSZ3 yields OTZ 1511/sort 1755. The
  cross-object order witness is dark node `0x800C2FF4` versus red node `0x800C8D84`/object
  `0x801E18B0`, for which frame-wide authored order preserves the retail winner.
- Widescreen currently works in measured Crashball and startup scenes but needs broader mode coverage.
  Interpolation rebuilds midpoint model geometry from consecutive immutable snapshots and remains
  partial until its source families and gameplay coverage are complete.

Detailed provenance remains in `docs/findings/`, `docs/issues/`, and `docs/info/`. Those records are
evidence inputs, not permission to restore a generated gameplay path.
