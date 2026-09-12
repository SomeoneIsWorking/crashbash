#include "boot_image.h"

#include "boot_image_identity.h"
#include "core.h"
#include "guest_execution.h"
#include "image_content_identity.h"
#include "image_identity.h"
#include "invalidation.h"

#include <cstddef>
#include <cstdint>
#include <lucent/content.h>
#include <lucent/log.h>
#include <span>
#include <stdexcept>

namespace crashbash {
namespace {

constexpr std::uint32_t kSectorBytes = 2048u;
constexpr std::uint32_t kRamBytes = 0x200000u;
constexpr std::uint32_t kPhysicalLoad = boot_image::kLoadAddress & 0x1fffffffu;
constexpr GuestAddressRange kImageRange{kPhysicalLoad, kPhysicalLoad + boot_image::kPayloadBytes};

static_assert(boot_image::kPayloadBytes == boot_image::kSectorCount * kSectorBytes);
static_assert(kPhysicalLoad <= kRamBytes && boot_image::kPayloadBytes <= kRamBytes - kPhysicalLoad);
static_assert(boot_image::kPayloadBytes >= sizeof(std::uint32_t));
static_assert(boot_image::kEntryPointerOffset <= boot_image::kPayloadBytes - sizeof(std::uint32_t));
static_assert((boot_image::kEntry & 0x1fffffffu) >= kImageRange.begin &&
              (boot_image::kEntry & 0x1fffffffu) < kImageRange.end);

runtime::GuestExecution &execution(Core &core) {
  if (!core.gameCtx) {
    throw std::logic_error("Crash Bash BOOT publication requires the title's Core context");
  }
  return *static_cast<runtime::GuestExecution *>(core.gameCtx);
}

} // namespace

bool isBootImageRead(std::uint32_t lba, std::uint32_t destination, std::uint32_t sectorCount) {
  return lba == boot_image::kDiscLba && destination == boot_image::kLoadAddress &&
         sectorCount == boot_image::kSectorCount;
}

BootImageReadResult
completeBootImageRead(Core &core, std::uint32_t lba, std::uint32_t destination, std::uint32_t sectorCount) {
  if (!isBootImageRead(lba, destination, sectorCount)) {
    return BootImageReadResult::Unrelated;
  }
  runtime::retireAuthenticatedImagesForWrite(core, kImageRange);
  const std::span<const std::uint8_t> bytes(core.ram + kPhysicalLoad, boot_image::kPayloadBytes);
  const auto digest = lucent::content::sha256(std::as_bytes(bytes));
  const auto actualSha = lucent::content::sha256_hex(digest);
  std::uint32_t entry = 0;
  for (std::uint32_t index = 0; index < sizeof(entry); ++index) {
    entry |= static_cast<std::uint32_t>(bytes[boot_image::kEntryPointerOffset + index]) << (index * 8u);
  }
  if (actualSha != boot_image::kSha256 || entry != boot_image::kEntry) {
    lucent::error("crashbash-boot",
                  "completed BOOT read failed image authentication: sha256 {} entry 0x{:08X}",
                  actualSha,
                  entry);
    return BootImageReadResult::Rejected;
  }
  psx::cpu::notifyExecutableWrite(core, kImageRange, psx::cpu::ExecutableWriteSource::ModuleLoad);
  const auto image = core.imageCatalog().activate("crashbash-usa-boot", kImageRange, imageContentIdentity(digest));
  execution(core).bindAuthenticatedImage(runtime::GuestImage::Boot, image, kImageRange);
  lucent::info("crashbash-boot",
               "authenticated BOOT image generation {} at 0x{:08X}",
               image.generation,
               boot_image::kLoadAddress);
  return BootImageReadResult::Published;
}

} // namespace crashbash
