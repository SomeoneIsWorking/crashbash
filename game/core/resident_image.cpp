#include "resident_image.h"

#include "executable_identity.h"

#include <lucent/content.h>

namespace crashbash {

psx::cpu::PsxExeLoadResult loadResidentImage(Core &core, std::span<const std::uint8_t> bytes) {
  if (bytes.size() != identity::kExecutableBytes) {
    return {std::nullopt, {}, "Crash Bash USA executable size does not match the authenticated manifest"};
  }
  if (lucent::content::sha256_hex(lucent::content::sha256(std::as_bytes(bytes))) != identity::kExecutableSha256) {
    return {std::nullopt, {}, "Crash Bash USA executable SHA-256 does not match the authenticated manifest"};
  }
  return psx::cpu::loadPsxExeImage(core, bytes, "crashbash-usa-resident");
}

} // namespace crashbash
