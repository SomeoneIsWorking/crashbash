#pragma once

#include "guest_program_image.h"

#include <cstdint>

namespace crashbash::guest {

// Recovered SCUS_945.70 guest ownership facts. Exact executable identity is authenticated before
// these addresses are used; the shared CRT0 auditor verifies the boot group against resident bytes.
inline constexpr GuestAddressRange kVSync{0x800320ECu, 0x800320F0u};

inline constexpr std::uint32_t kGameMain = 0x8002718Cu;
inline constexpr std::uint32_t kApplicationMain = 0x80010158u;
inline constexpr std::uint32_t kCdFileRead = 0x80027790u;
inline constexpr std::uint32_t kCdLicenseStartup = 0x8002D4F4u;
inline constexpr std::uint32_t kDisplayFrame = 0x800272ACu;
inline constexpr std::uint32_t kGpuTimeoutArm = 0x8003126Cu;
inline constexpr std::uint32_t kGpuTimeoutCheck = 0x800312A0u;
inline constexpr std::uint32_t kGpuTransfer = 0x8003165Cu;
inline constexpr std::uint32_t kGteInitialization = 0x80033494u;
inline constexpr std::uint32_t kCdDriveReady = 0x800349ACu;
inline constexpr std::uint32_t kCdInitHandshake = 0x80034B8Cu;
inline constexpr std::uint32_t kCdSearchFile = 0x80034C6Cu;
inline constexpr std::uint32_t kCdSync = 0x8003E6B0u;
inline constexpr std::uint32_t kCdCommand = 0x8003EBF8u;
inline constexpr std::uint32_t kMemoryCardStartup = 0x800486DCu;

// The resident load queue's pacing pump. `kLoadPump` advances ONE queue step per call — tick the
// hard-coded inter-read cooldown, poll-complete the active read, or start the next queued read —
// and the title pumps it once per frame, so a queued chain costs several frames per file even
// though this port's disc read (kCdFileRead) completes synchronously. Retail's pending counter at
// 0x80012FFC, which the scene-machine loading gates read, is (queue head != 0) + cooldown +
// read-active — zero exactly when the three pump-owned words below are zero. The retail swap path
// at 0x8001E610 completes a chain by looping `kLoadPump` while that counter is nonzero; the drain
// owner loops the same way from those words, because the original-call seam only exists at
// installed owners and 0x80012FFC is ordinary guest code.
inline constexpr std::uint32_t kLoadPump = 0x8001231Cu;
inline constexpr std::uint32_t kLoadCooldownWord = 0x80050628u;
inline constexpr std::uint32_t kLoadReadActiveWord = 0x8005062Cu;
inline constexpr std::uint32_t kLoadQueueHeadWord = 0x80050634u;

// BOOT overlay logo controller and the resident scene-transition primitive it reaches once the
// logos have completed. The controller's natural completion requests kBootLogoHandoffState with
// kBootLogoHandoffFlags; native Start handling uses the same dispatcher-owned request instead of
// advancing the logo timer or mutating the active scene.
inline constexpr std::uint32_t kBootLogoUpdate = 0x8008E5BCu;
inline constexpr std::uint32_t kSceneTransitionRequest = 0x8001E588u;
inline constexpr std::uint32_t kBootLogoHandoffState = 0x800A00DCu;
inline constexpr std::uint32_t kBootLogoHandoffFlags = 0x00000012u;

inline constexpr std::uint32_t kInitialProcessState = 0x8004E0B8u;
inline constexpr std::uint32_t kCurrentProcessState = 0x8005B648u;
inline constexpr std::uint32_t kInitialStateEnter = 0x80010410u;
inline constexpr std::uint32_t kInitialStateUpdate = 0x80010394u;
inline constexpr std::uint32_t kInitialStatePresent = 0x80010278u;

// The application shell state (kInitialProcessState) dispatches enter/update/present through a
// second, nested scene object. kAppModeVtable holds the POINTER to it; the three handlers are its
// words +0/+4/+8 and +0x10 is the arena the scene owns.
//
// kAppModeVtable is the shell's OWN root scene and is NOT a mode selector: its single writer is
// `0x800101CC` in the resident application main, with $s0 = 0x80078C90 (BOOT's load address), and
// that one write happens before any mode exists. BOOT's own update `0x80092BA0` is
// `func_0x8001e598(&DAT_8009f644); func_0x8001e610(&DAT_8009f658,&DAT_8009f644)` — so the mode
// machine is the SCENE MACHINE below and 0x80078C90 is the permanent root scene whose update runs
// it. Reading it tells you nothing about boot, menu or gameplay.
inline constexpr std::uint32_t kAppModeVtable = 0x8004E0DCu;

// The scene machine that actually selects boot / menu / attract / gameplay. `0x8001E610` reads it
// as {current, target, previous, flags} and `0x8001E588` writes target (+4) and flags (+0xC);
// BOOT's update passes it together with the four-word transition clock it ticks every frame.
// `kSceneTransition` is the machine; `kSceneTransitionClock` is {value, step, age, flags}.
inline constexpr std::uint32_t kSceneTransition = 0x8009F658u;
inline constexpr std::uint32_t kSceneTransitionClock = 0x8009F644u;
// A SCENE is {enter, update, present, arena}; the SCENE RECORD is {current, target, previous, flags}.
// Both start at word 0, which is why the shared slot names below are the scene's and the record's
// current scene has its own.
inline constexpr std::uint32_t kSceneTransitionEnterSlot = 0u;
inline constexpr std::uint32_t kSceneTransitionUpdateSlot = 4u;
inline constexpr std::uint32_t kSceneTransitionPresentSlot = 8u;
inline constexpr std::uint32_t kSceneCurrentSlot = 0u;
inline constexpr std::uint32_t kSceneTargetSlot = 4u;
inline constexpr std::uint32_t kScenePreviousSlot = 8u;
inline constexpr std::uint32_t kSceneFlagsSlot = 12u;
inline constexpr std::uint32_t kSceneClockAgeSlot = 8u;

// The menu world runs a SECOND scene machine over this record: scene 0x8009F720's own update
// (FUN_80092EDC, in BOOT) ends in `0x8001E610(&DAT_8009F8A4, &DAT_8009F644)` — the retail runner,
// with the same transition clock as the outer machine — so it is {current, target, previous, flags}
// exactly as kSceneTransition is. Its CURRENT is the active menu screen's scene struct (SELECT
// GAME TYPE is 0x800B8E28; each screen has its own), and a screen's accept writes target (+4) and
// flags (+0xC) through FUN_8001E838/FUN_8001E848 — the two fields kSceneTransitionRequest writes.
// menu_boundary.cpp measured these same words as "current/pending manager"; they are a scene record.
inline constexpr std::uint32_t kMenuSceneTransition = 0x8009F8A4u;

// The LOADING card's draw, in the resident image: `FUN_80018B08` paints the animated panel (and
// its background quad) from the picture table, and BOOT's two handoff screens reach it from their
// own present handlers — `FUN_8009421C` for the screen record `0x8009F998` (the general handoff) and
// `FUN_8009414C` for `0x8009FA00` (the level-0x28 route). Both presents are entered only while the
// handoff scene `0x800A00DC` is current, so those two call sites are the whole of the card's
// presentation; everything else the handoff presents — the transition clock's fade word at
// `0x800569AC`, and the level frame the handoff has built — is left on the guest's own body.
inline constexpr std::uint32_t kLoadingCardDraw = 0x80018B08u;

// The arena the level setup reads: written by the scene dispatcher and read by `FUN_800793E8`,
// which indexes the arena table below with it. In MENU it is filled from the SELECT BATTLE TYPE
// cursor by `FUN_800b7ef4`.
//
// The dispatcher is `FUN_8007F314`, the update owner of the scene that hosts BOTH the menus and the
// match flow (`0x8009F720`; the scene table at `0x8009F724` holds that pointer as DATA, so only the
// scene machine calls it). Its $a1 is NOT the arena index — a measured call with $a1 = 4 stored
// 0x28 here and then faulted the guest at MENU.BIN 0x800B5A74, because the owner also expects the
// machine's prepared frame. The arena therefore reaches the dispatcher through the SELECT BATTLE TYPE
// accept's own flow-table entry (dev_arena.cpp).
inline constexpr std::uint32_t kArenaSelection = 0x8009E5DCu;
// Six-byte arena records: the first halfword is the level type `FUN_800793e8` switches on (0x104
// Crashball, 0x106..0x10E the arena families), the second the per-family slot, the third the game's
// own arena id. Adjacent in the resident image: the arena names ('CRATE CRUSH' 0x8004C78C,
// 'POGO PANDEMONIUM' 0x8004C7A4, 'BALLISTIX' 0x8004C7B8) hang off 0x8004DD84.
inline constexpr std::uint32_t kArenaTable = 0x8004DDD0u;
inline constexpr std::uint32_t kArenaRecordBytes = 6u;

// The menu's own arena-selection flow, all of it MENU-overlay state the game owns:
//
//   0x800B3CA8  SELECT GAME TYPE's update. Its accept branch installs the mode's screen-flow table
//               by calling 0x800B5360 with the ADDRESS of that mode's flow pointer.
//   0x800B5360  stores the flow-table address in 0x800B9620 and requests the transition from the
//               menu screen record 0x8009F8A4 to *flowTable. The menu machine then walks the table
//               one entry per screen accept: 0x800B5410 advances 0x800B9620 by a word and asks the
//               screen machine for the next entry while the word is non-zero.
//   0x800B5360 is two stores and no guest call, so execution never returns to its boundary.
inline constexpr std::uint32_t kModeAccept = 0x800B5360u;
inline constexpr std::uint32_t kModeAcceptReturnPc = 0x800B53C8u;
// A mode's flow table is a RUN of screen records in MENU that starts at SELECT GAME TYPE. Measured
// in MENU.BIN: the Battle run is [0x800B8E28, 0x800B8E3C, 0x800B9DF4, 0x800BA72C, 0x800BAAB4,
// 0x800B8E8C, 0x800B8EAC] at 0x800B8EA0 and the Tournament run is [0x800B8E28, 0x800B8E3C,
// 0x800B9DF4, 0x8009EB84, ...] at 0x800B8ED0. The run that reaches the match through SELECT BATTLE
// TYPE is the player's battle route; the run at 0x800B8EC0 ([0x800B8E50, ...]) is the attract
// demo's short flow, and entering that one skips the arena screen entirely — which is why the flow
// table is LOCATED by the screen its fourth entry reaches rather than named by address.
inline constexpr std::uint32_t kSelectBattleTypeBattleScreen = 0x800BA72Cu;
inline constexpr std::uint32_t kTournamentMatchScreen = 0x8009EB84u;
// SELECT BATTLE TYPE has TWO screen records with the same body — 0x800BA72C and 0x800BAF2C, both
// enter 0x800B6CF4, update 0x800B7458, present 0x800B79A8 — and the arena screen is recognised by
// its update pointer rather than by an address picked here.
inline constexpr std::uint32_t kSelectBattleTypeUpdate = 0x800B7458u;
inline constexpr std::uint32_t kSelectBattleTypeEnter = 0x800B6CF4u;
// The MENU overlay's mapped range, from titles/crashbash/menu_module.json. A page of the arena table
// is the game's own only when its sub-table pointer lies inside this resident image.
inline constexpr std::uint32_t kMenuImageBase = 0x800B32B4u;
inline constexpr std::uint32_t kMenuImageBytes = 0x00008000u;
inline constexpr std::uint32_t kModeCursor = 0x800B95F0u;
inline constexpr std::uint32_t kModeCursorBattle = 0u;
inline constexpr std::uint32_t kModeCursorTournament = 2u;
inline constexpr std::uint32_t kArenaCursorPage = 0x8005A64Au;
inline constexpr std::uint32_t kArenaCursorIndex = 0x8005A64Bu;
inline constexpr std::uint32_t kArenaPageTable = 0x800BA324u;
inline constexpr std::uint32_t kArenaPageCounts = 0x800BA328u;
inline constexpr std::uint32_t kArenaEntryBytes = 0x10u;
inline constexpr std::uint32_t kArenaPageStride = 0x8u;

inline constexpr std::uint32_t kDisplayFieldsPerFrame = 0x8004E0E0u;
inline constexpr std::uint32_t kVblankCounter = 0x8006D8DCu;
inline constexpr std::uint32_t kVblankRoot = 0x8003ADD4u;
inline constexpr std::uint32_t kCdReadActive = 0x800637B4u;
inline constexpr std::uint32_t kCdBaseLba = 0x800637B8u;
inline constexpr std::uint32_t kCdLicenseState = 0x80067894u;
inline constexpr std::uint32_t kDiscExecutableLba = 0x00000017u;
inline constexpr std::uint32_t kDiscExecutableSize = 0x00069800u;
inline constexpr std::uint32_t kDiscSystemCnfLba = 0x000000EAu;
inline constexpr std::uint32_t kDiscSystemCnfSize = 0x00000045u;
inline constexpr std::uint32_t kDiscDataLba = 0x000000ECu;
inline constexpr std::uint32_t kDiscDataSize = 0x045D4000u;
inline constexpr std::uint32_t kDiscTrackSectors = 0x000127FEu;

} // namespace crashbash::guest
