#pragma once

#include "psx_exe_image.h"

namespace crashbash {

// Requires the exact title manifest fingerprint before publishing the same bytes through psxport.
psx::cpu::PsxExeLoadResult loadResidentImage(Core &core, std::span<const std::uint8_t> bytes);

} // namespace crashbash
