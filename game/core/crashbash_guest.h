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
// MEASURED, and this corrects the reading issue 0032 recorded: this pointer is the shell's OWN
// scene and is not a mode selector. Its single writer is `0x800101CC` (`sw $s0, -0x1f24($v0)` in
// the resident application main, with $s0 = 0x80078C90 = BOOT's load address), and that one write
// happens before any mode exists. BOOT's own update `0x80092BA0` is
// `func_0x8001e598(&DAT_8009f644); func_0x8001e610(&DAT_8009f658,&DAT_8009f644)` — i.e. the mode
// machine is the SCENE MACHINE below, and 0x80078C90 is the permanent root scene whose update runs
// it. So a run that never changes this pointer has told us nothing about boot / menu / gameplay,
// which is what the frame driver used to report.
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
