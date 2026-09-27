#pragma once

#include "guest_execution.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

class Core;

namespace crashbash {

struct AuthenticatedModuleSpec {
  runtime::GuestImage image;
  std::string_view name;
  std::uint32_t discLba;
  std::uint32_t loadAddress;
  std::uint32_t sectorCount;
  std::size_t payloadBytes;
  std::string_view sha256;
  std::uint32_t entryPointerOffset;
  std::uint32_t entry;
};

// The entry word is a SECOND, independent content witness, and only some modules carry one.
//
// MEASURED 2026-09-27, from the provisioned overlays. BOOT's payload begins with its own three
// function pointers, and its manifest records entry_pointer_offset 0 -> 0x80092BDC; MENU records
// 0x6270 -> 0x800B5244. The five nested gameplay overlays record `null` for BOTH fields, and that is
// a measured property of those images rather than a gap in the manifest: DAT28272's first eight words
// decode as ASCII ("BREATH", "JUMP", "DAZE", "WIN" — the animation-state name table the project
// state doc already attributes to this module), and DAT22510/DAT28136/DAT28241/DAT28382 are the same
// shape. A module whose header is a string table has no table-of-contents pointer to compare, so for
// those modules the SHA-256 over the whole payload is the ENTIRE content witness, and the
// authentication log says which witness it used rather than implying a second one existed.
//
// A zero `entry` is therefore a stated absence, not a placeholder: `hasEntryWitness` is a property of
// the specification the manifest produced, and nothing infers one.
constexpr bool hasEntryWitness(const AuthenticatedModuleSpec &spec) {
  return spec.entry != 0u;
}

enum class ModuleImageReadResult { Unrelated, Published, Rejected };

constexpr bool validModuleSpec(const AuthenticatedModuleSpec &spec) {
  constexpr std::size_t kSectorBytes = 2048u;
  constexpr std::uint32_t kRamBytes = 0x200000u;
  const std::uint32_t physical = spec.loadAddress & 0x1fffffffu;
  const std::uint32_t physicalEntry = spec.entry & 0x1fffffffu;
  return !spec.name.empty() && spec.sectorCount > 0u && spec.payloadBytes == spec.sectorCount * kSectorBytes &&
         physical < kRamBytes && spec.payloadBytes <= kRamBytes - physical &&
         spec.payloadBytes >= sizeof(std::uint32_t) && spec.sha256.size() == 64u &&
         (!hasEntryWitness(spec) || (spec.entryPointerOffset <= spec.payloadBytes - sizeof(std::uint32_t) &&
                                     physicalEntry >= physical && physicalEntry < physical + spec.payloadBytes));
}

bool isModuleImageRead(const AuthenticatedModuleSpec &spec,
                       std::uint32_t lba,
                       std::uint32_t destination,
                       std::uint32_t sectorCount);
ModuleImageReadResult completeModuleImageRead(Core &core,
                                              const AuthenticatedModuleSpec &spec,
                                              std::uint32_t lba,
                                              std::uint32_t destination,
                                              std::uint32_t sectorCount);

} // namespace crashbash
