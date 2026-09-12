#include "authenticated_module_image.h"

#include "core.h"
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

bool isModuleImageRead(const AuthenticatedModuleSpec &spec,
                       std::uint32_t lba,
                       std::uint32_t destination,
                       std::uint32_t sectorCount) {
  return lba == spec.discLba && destination == spec.loadAddress && sectorCount == spec.sectorCount;
}

ModuleImageReadResult completeModuleImageRead(Core &core,
                                              const AuthenticatedModuleSpec &spec,
                                              std::uint32_t lba,
                                              std::uint32_t destination,
                                              std::uint32_t sectorCount) {
  if (!validModuleSpec(spec)) {
    throw std::logic_error("Crash Bash loaded-module specification is invalid");
  }
  if (!isModuleImageRead(spec, lba, destination, sectorCount)) {
    return ModuleImageReadResult::Unrelated;
  }
  if (!core.gameCtx) {
    throw std::logic_error("Crash Bash loaded-module publication requires the title's Core context");
  }
  const std::uint32_t physical = spec.loadAddress & 0x1fffffffu;
  const GuestAddressRange range{physical, static_cast<std::uint32_t>(physical + spec.payloadBytes)};
  runtime::retireAuthenticatedImagesForWrite(core, range);
  const std::span<const std::uint8_t> bytes(core.ram + physical, spec.payloadBytes);
  const auto digest = lucent::content::sha256(std::as_bytes(bytes));
  const auto actualSha = lucent::content::sha256_hex(digest);
  std::uint32_t entry = 0u;
  for (std::uint32_t index = 0; index < sizeof(entry); ++index) {
    entry |= static_cast<std::uint32_t>(bytes[spec.entryPointerOffset + index]) << (index * 8u);
  }
  if (actualSha != spec.sha256 || entry != spec.entry) {
    lucent::error("crashbash-module",
                  "completed {} read failed image authentication: sha256 {} entry 0x{:08X}",
                  spec.name,
                  actualSha,
                  entry);
    return ModuleImageReadResult::Rejected;
  }
  psx::cpu::notifyExecutableWrite(core, range, psx::cpu::ExecutableWriteSource::ModuleLoad);
  const auto image = core.imageCatalog().activate(spec.name, range, imageContentIdentity(digest));
  auto &execution = *static_cast<runtime::GuestExecution *>(core.gameCtx);
  execution.bindAuthenticatedImage(spec.image, image, range);
  lucent::info("crashbash-module",
               "authenticated {} image generation {} at 0x{:08X}",
               spec.name,
               image.generation,
               spec.loadAddress);
  return ModuleImageReadResult::Published;
}

} // namespace crashbash
