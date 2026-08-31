#pragma once

#include "guest_program_image.h"

#include <cstdint>

namespace crashbash::guest {

// Verified SCUS_945.70 guest ownership facts. tools/verify_native_ownership.py compares these
// shipping values to the retail executable and to the generated call sites that established them.
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
inline constexpr std::uint32_t kSceneTransition = 0x8009F658u;
inline constexpr std::uint32_t kBootLogoHandoffState = 0x800A00DCu;
inline constexpr std::uint32_t kBootLogoHandoffFlags = 0x00000012u;

inline constexpr std::uint32_t kInitialProcessState = 0x8004E0B8u;
inline constexpr std::uint32_t kCurrentProcessState = 0x8005B648u;
inline constexpr std::uint32_t kInitialStateEnter = 0x80010410u;
inline constexpr std::uint32_t kInitialStateUpdate = 0x80010394u;
inline constexpr std::uint32_t kInitialStatePresent = 0x80010278u;

// The application shell state (kInitialProcessState) dispatches enter/update/present through a
// second, nested mode object. kAppModeVtable holds the POINTER to it; the three handlers are its
// words +0/+4/+8. This is the machine that actually selects boot / menu / gameplay, so a run that
// never changes this pointer is not running the game, however healthy the outer frame loop looks.
inline constexpr std::uint32_t kAppModeVtable = 0x8004E0DCu;

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
