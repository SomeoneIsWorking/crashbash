#pragma once

#include <cstdint>

class Core;

namespace crashbash {

void registerCdFileReadOverride(Core &core);
// Retire image generations touched by one mapped 2048-byte CD sector before copying its bytes.
void retireImagesForCdSectorWrite(Core &core, std::uint32_t destination);

} // namespace crashbash
