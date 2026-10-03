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

// The two heap packet pools, named by the guest globals the retail renderer's setup functions
// publish. Parity 0 is `0x8005F790`/`0x8005F794`; parity 1 is `0x8006379C`/`0x800637A0`. Each pair is
// {base, end}; the "current" pointer that walks the pool is `0x8005F798` / `0x800637A4` and is NOT a
// window bound — the framework needs where the pool STARTS and STOPS, not where it is.
constexpr GuestPacketPoolWindows kPacketPoolWindows{
    .representation = GuestPacketPoolWindows::Representation::LiveBaseEndPointers,
    .basePointer = {0x8005F790u, 0x8006379Cu},
    .endPointer = {0x8005F794u, 0x800637A0u},
};

} // namespace

psx::cpu::PsxExeLoadResult TitleAdapter::loadExecutable(Core &core, std::span<const std::uint8_t> bytes) {
  if (core.runtime != this || !core.gameCtx) {
    return {std::nullopt, {}, "Crash Bash executable load requires this title's Core context"};
  }
  auto &execution = *static_cast<runtime::GuestExecution *>(core.gameCtx);
  auto result = loadResidentImage(core, bytes, execution.ledger());
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

diagnostics::RunLedger &TitleAdapter::runLedger(Core &core) const {
  return runtime::runLedgerFor(core);
}

const GuestProgramImage *TitleAdapter::guestProgramImage() const {
  return &kProgramImage;
}
const PlatformHlePlan *TitleAdapter::platformHlePlan() const {
  return &kPlatformPlan;
}

// THE 2D PACKET POOL, as the guest globals the retail renderer publishes.
//
// The retail renderer uses no fixed packet arena: `0x800274FC` and `0x800276C4` each call the guest
// heap allocator for `requested_size + 0x1800` and publish inclusive base / exclusive end for one
// parity, and `0x800272AC` alternates between the two. The window is therefore declared in
// psxport's LIVE form — two pointer-global pairs — and the bounds are re-read whenever the guest
// rewrites a global, which is what tracks a pool the game reallocates. The heap ADDRESSES are
// deliberately not here: they are observed values, not title constants.
//
// The declaration is what makes OtAttr able to attribute a guest packet to the producer that wrote
// it, and `GuestPacketFilter` suppresses only what OtAttr attributes. With no window declared the
// filter matches nothing, and the guest's own GP0 replay draws a second, centred copy of every HUD
// element the native producer also draws.
const GuestPacketPoolWindows *TitleAdapter::guestPacketPoolWindows() const {
  return &kPacketPoolWindows;
}

const char *TitleAdapter::discEnvVar() const {
  return "PSXPORT_CRASHBASH_DISC";
}

psx::state::NativeStatePort *TitleAdapter::nativeState(Core &core) const {
  if (core.runtime != this || !core.gameCtx) {
    return nullptr;
  }
  return &static_cast<runtime::GuestExecution *>(core.gameCtx)->statePort();
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

bool TitleAdapter::controlCommand(Core &core, const char *cmd, const char *line, FILE *out) {
  // The title's own control-channel surface. Only the developer's `arena` command lives here today;
  // player-facing commands and player input never reach it.
  return frameDriver(core).devArena().handle(core, cmd, line, out);
}

std::unique_ptr<TemporalFramePresentation> TitleAdapter::createTemporalFramePresentation(Game &game) {
  return std::make_unique<render::InterpolatedScenePresentation>(game);
}
std::unique_ptr<FrameDriver> TitleAdapter::createFrameDriver(Game &game) {
  return std::make_unique<CrashBashFrameDriver>(game);
}

} // namespace crashbash
