#pragma once

#include <cstdint>
#include <functional>
#include <optional>

namespace crashbash::render {

using ModelFrameWordReader = std::function<std::optional<std::uint32_t>(std::uint32_t)>;

// Resolves the source frame record selected by 0x80019A60. Family 0x2000 addresses the inline
// frame table directly; family 0x5000 follows the title's two-level 0x800159C4/0x8001DD20 table.
// The reader owns address validation so this remains the one production/test implementation.
std::optional<std::uint32_t>
resolveModelFrameSource(std::uint16_t frameCode, std::uint32_t modelData, const ModelFrameWordReader &readWord);

} // namespace crashbash::render
