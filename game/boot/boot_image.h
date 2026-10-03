#pragma once

#include "authenticated_module_image.h"

#include <cstdint>

class Core;

namespace crashbash {

using BootImageReadResult = ModuleImageReadResult;

// The retail CD read owns the bytes. This owner admits only the exact measured complete BOOT read
// and authenticates the resulting guest RAM before publishing its executable generation.
bool isBootImageRead(std::uint32_t lba, std::uint32_t destination, std::uint32_t sectorCount);
BootImageReadResult
completeBootImageRead(Core &core, std::uint32_t lba, std::uint32_t destination, std::uint32_t sectorCount);

} // namespace crashbash
