#pragma once

#include <cstddef>
#include <cstdint>
#include <lucent/content.h>

namespace crashbash {

// ImageCatalog uses a compact content tag beside its unique per-load generation. Admission always
// checks the complete SHA-256 first; the tag is the canonical leading 64 bits of that digest.
inline std::uint64_t imageContentIdentity(const lucent::content::Sha256 &sha) {
  std::uint64_t prefix = 0u;
  for (std::size_t index = 0; index < sizeof(prefix); ++index) {
    prefix = (prefix << 8u) | sha[index];
  }
  return prefix;
}

} // namespace crashbash
