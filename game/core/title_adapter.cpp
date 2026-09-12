#include "title_adapter.h"

#include "boot_image_identity.h"
#include "core.h"
#include "crashbash_boot.h"
#include "crashbash_frame_driver.h"
#include "crashbash_guest.h"
#include "executable_identity.h"
#include "game.h"
#include "guest_execution.h"
#include "image_content_identity.h"
#include "image_identity.h"
#include "interpolated_scene.h"
#include "memcard.h"
#include "native_owner_set.h"
#include "platform_hle.h"
#include "resident_image.h"

#include <lucent/content.h>
#include <lucent/log.h>
#include <stdexcept>

namespace crashbash {
namespace {

// Recovered SCUS_945.70 CRT0 group, retained from the handwritten pre-migration title facts.
// psxport's crt0_audit re-derives and compares these against the authenticated guest bytes at boot.
constexpr std::uint32_t kBssBegin = 0x8006E9F0u;
constexpr GuestAddressRange kResidentCodeRange{identity::kTextAddress & 0x1fffffffu, kBssBegin & 0x1fffffffu};
static_assert(kResidentCodeRange.begin < kResidentCodeRange.end);
static_assert(kResidentCodeRange.end <= (identity::kTextAddress & 0x1fffffffu) + identity::kTextBytes);

constexpr GuestProgramImage kProgramImage{
    .bss = {kBssBegin, boot_image::kLoadAddress},
    .stackTopWordAddress = 0x8002E860u,
    .stackReserveWordAddress = 0x8006D8B4u,
    .heapBase = boot_image::kLoadAddress,
    .globalPointer = 0x8006E9ECu,
    .libcInitEntry = 0x8003ACCCu,
    .gameMainEntry = guest::kGameMain,
    .crt0Entry = identity::kEntry,
    .residentText = kResidentCodeRange,
    .stackBias = {true, 0},
};

constexpr PlatformHlePlan kPlatformPlan{
    .cdCommandAddress = guest::kCdCommand,
    .cdSyncAddress = guest::kCdSync,
    .cdSearchFileAddress = guest::kCdSearchFile,
    .vsyncAddress = guest::kVSync.begin,
    .windowLo = {guest::kVSync.begin, guest::kCdInitHandshake},
    .windowHi = {guest::kVSync.end, guest::kCdCommand + 4u},
};

} // namespace

psx::cpu::PsxExeLoadResult TitleAdapter::loadExecutable(Core &core, std::span<const std::uint8_t> bytes) {
  if (core.runtime != this || !core.gameCtx) {
    return {std::nullopt, {}, "Crash Bash executable load requires this title's Core context"};
  }
  auto &execution = *static_cast<runtime::GuestExecution *>(core.gameCtx);
  auto result = loadResidentImage(core, bytes);
  if (result) {
    // The EXE header includes BSS and the heap tail in its text byte count. Those bytes were
    // authenticated and loaded, but only the pre-BSS interval is resident executable code.
    runtime::retireAuthenticatedImagesForWrite(core, result.image.physicalText);
    if (!core.imageCatalog().deactivate(*result.identity)) {
      throw std::logic_error("Crash Bash resident loader lost its freshly published image");
    }
    const auto digest = lucent::content::sha256(std::as_bytes(bytes));
    const auto image =
        core.imageCatalog().activate("crashbash-usa-resident-code", kResidentCodeRange, imageContentIdentity(digest));
    result.identity = image;
    execution.bindAuthenticatedImage(runtime::GuestImage::Resident, image, kResidentCodeRange);
  }
  return result;
}

const GuestProgramImage *TitleAdapter::guestProgramImage() const {
  return &kProgramImage;
}
const PlatformHlePlan *TitleAdapter::platformHlePlan() const {
  return &kPlatformPlan;
}
const char *TitleAdapter::discEnvVar() const {
  return "PSXPORT_CRASHBASH_DISC";
}

void *TitleAdapter::createContext(Core &core) {
  return new runtime::GuestExecution(core);
}
void TitleAdapter::destroyContext(void *context) {
  delete static_cast<runtime::GuestExecution *>(context);
}

void TitleAdapter::registerOverrides(Game &game) {
  // The retail libmcrd walks the BIOS device table itself; publishing only file callbacks leaves
  // its completion wait permanently pending. Preserve the actual device installation contract.
  card_overrides_init(&game);
  registerNativeOwners(game.core);
}

void TitleAdapter::bootInit(Core &core) {
  lucent::info("boot", "executing finite Crash Bash boot prefix; native FrameDriver owns repetition");
  runBootPrefix(core);
}

RenderCapabilities TitleAdapter::renderCapabilities() const {
  return titleRenderCapabilities();
}

bool TitleAdapter::guestVramIsPicture(const Game &) const {
  // Retained authored uploads supply the SCEA boot picture and native-scene backdrops.
  return true;
}

std::unique_ptr<TemporalFramePresentation> TitleAdapter::createTemporalFramePresentation(Game &game) {
  return std::make_unique<render::InterpolatedScenePresentation>(game);
}
std::unique_ptr<FrameDriver> TitleAdapter::createFrameDriver(Game &game) {
  return std::make_unique<CrashBashFrameDriver>(game);
}

} // namespace crashbash
