#include "title_adapter.h"

#include "core.h"
#include "crashbash_boot.h"
#include "crashbash_frame_driver.h"
#include "crashbash_guest.h"
#include "executable_identity.h"
#include "game.h"
#include "guest_execution.h"
#include "interpolated_scene.h"
#include "memcard.h"
#include "native_owner_set.h"
#include "platform_hle.h"
#include "resident_image.h"

#include <lucent/log.h>

namespace crashbash {
namespace {

// Recovered SCUS_945.70 CRT0 group, retained from the handwritten pre-migration title facts.
// psxport's crt0_audit re-derives and compares these against the authenticated guest bytes at boot.
constexpr GuestProgramImage kProgramImage{
    .bss = {0x8006E9F0u, 0x80078C90u},
    .stackTopWordAddress = 0x8002E860u,
    .stackReserveWordAddress = 0x8006D8B4u,
    .heapBase = 0x80078C90u,
    .globalPointer = 0x8006E9ECu,
    .libcInitEntry = 0x8003ACCCu,
    .gameMainEntry = guest::kGameMain,
    .crt0Entry = identity::kEntry,
    .residentText = {identity::kTextAddress & 0x1FFFFFFFu,
                     (identity::kTextAddress & 0x1FFFFFFFu) + identity::kTextBytes},
    .stackBias = {true, 0},
};

constexpr PlatformHlePlan kPlatformPlan{
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
  const auto previous = execution.activeKey(runtime::GuestImage::Resident, identity::kEntry);
  auto result = loadResidentImage(core, bytes);
  if (result) {
    execution.bindAuthenticatedImage(runtime::GuestImage::Resident, *result.identity, result.image.physicalText);
    if (previous) {
      core.imageCatalog().deactivate(previous->image);
    }
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
