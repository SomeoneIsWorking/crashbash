#include "cd_file_read.h"

#include "core.h"
#include "crashbash_guest.h"
#include "disc.h"
#include "game.h"
#include "guest_execution.h"

#include <array>
#include <cstdint>
#include <lucent/log.h>

namespace crashbash {
namespace {

constexpr std::uint32_t kSectorBytes = 2048u;

void cdFileReadOwned(Core *core) {
  // Retail 0x80027790 starts an interrupt-driven 2048-byte-sector read, then returns its async
  // completion state. The shipping port has no asynchronous CD controller: complete the same
  // descriptor-relative interval from the real native disc before returning. Success is reported
  // only after every requested sector has been copied into guest RAM.
  const std::uint32_t descriptor = core->r[4];
  const std::uint32_t descriptorOffset = core->r[5];
  const std::uint32_t destination = core->r[6];
  const std::uint32_t sectorCount = core->r[7];
  const std::uint32_t lba = core->mem_r32(descriptor) + descriptorOffset + core->mem_r32(guest::kCdBaseLba);
  std::array<std::uint8_t, kSectorBytes> sector{};

  for (std::uint32_t index = 0; index < sectorCount; ++index) {
    if (!disc_read_sector(&core->game->disc, lba + index, sector.data())) {
      core->mem_w32(guest::kCdReadActive, 0u);
      core->r[2] = 0xFFFFFFFFu;
      lucent::error("crashbash-cd", "native file read failed at LBA {} ({}/{})", lba + index, index, sectorCount);
      return;
    }
    const std::uint32_t sectorDestination = destination + index * kSectorBytes;
    for (std::uint32_t byte = 0; byte < kSectorBytes; ++byte) {
      core->mem_w8(sectorDestination + byte, sector[byte]);
    }
  }

  core->mem_w32(guest::kCdReadActive, 0u);
  core->r[2] = 0u;
  lucent::debug("crashbash-cd",
                "native file read completed: {} sector(s) from LBA {} to 0x{:08X}",
                sectorCount,
                lba,
                destination);
}

} // namespace

void registerCdFileReadOverride(Core &core) {
  runtime::registerNativeOverride(
      core, runtime::GuestImage::Resident, guest::kCdFileRead, "CrashBash::CdFileRead", cdFileReadOwned);
}

} // namespace crashbash
