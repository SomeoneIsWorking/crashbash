#pragma once

#include "authenticated_module_image.h"

#include <cstdint>
#include <span>

class Core;

namespace crashbash {

// The nested gameplay overlays share ONE load address, 0x800B32B4, with the initial menu module, so
// publishing one necessarily retires the previous occupant's coverage over the bytes it rewrote.
// Before this owner existed, only BOOT and MENU were ever published, so the FIRST nested load left
// its own code with no image identity at all and the executor refused it as a typed fault.
//
// MEASURED 2026-09-27, on the provisioned image and a 1500-frame headless run: the boot logo
// sequence completes and the guest then reads `38 sector(s) from LBA 28272 to 0x800B32B4` — exactly
// `titles/crashbash/dat28272_module.json` (payload_disc_lba 28272, sector_count 38, load_address
// 0x800B32B4) — and the very next dispatch of guest address 0x800C3434 fails with "resolves to zero
// or multiple active code images". 0x800C3434 is inside DAT28272's payload range
// 0x800B32B4..0x800C62B4 and is its INITIALIZER: it installs its own behaviour table at 0x8005AA70,
// which is the address docs/project-state.md already attributes to this module.
//
// This owner holds the tracked specifications and offers a completed read to the ONE existing
// authentication/publication implementation (`completeModuleImageRead`). It adds no second copy of
// the digest, the retirement, the invalidation or the binding, and it never writes guest bytes.
class NestedModuleImage {
public:
  // Every tracked occupant of the nested slot, in manifest order. `nestedModuleSpecs()` is the
  // single list a caller can enumerate, so the count a report quotes has one source.
  static std::span<const AuthenticatedModuleSpec> specs();
  static std::size_t specCount();

  // TRUE when this exact (lba, destination, sectorCount) triple is one of the tracked modules. A
  // rejection is not implied: the offer below is what decides, and it authenticates the bytes.
  static bool isNestedModuleRead(std::uint32_t lba, std::uint32_t destination, std::uint32_t sectorCount);

  // Offer one completed CD read to the tracked nested modules. `Unrelated` means the read belongs to
  // no tracked module (the overwhelming majority — every asset load), `Published` means a module was
  // authenticated and bound, and `Rejected` means the read matched a specification but the bytes in
  // guest RAM are not that module, which the caller must report as a failed read.
  static ModuleImageReadResult
  offer(Core &core, std::uint32_t lba, std::uint32_t destination, std::uint32_t sectorCount);
};

} // namespace crashbash
