#pragma once

#include <cstdint>

namespace crashbash::render {

// DisplayFrame alternates two 0x1000-word ordering tables. A producer records the render list it
// inserted into: the base of its viewport's slice inside one table (see render_viewport.h), not that
// table's base address.
constexpr std::uint32_t kOrderingTableWordCount = 0x1000u;

constexpr bool spriteRenderListTargetsOrderingTable(std::uint32_t renderList, std::uint32_t orderingTable) {
  return renderList >= orderingTable && renderList - orderingTable < kOrderingTableWordCount * sizeof(std::uint32_t);
}

// The frame-wide position of an insertion at `bin` of `renderList`: the table word it lands in. The guest
// draws the table from its last word to its first, so this one number orders every producer across every
// viewport; a bin alone is relative to its own viewport's slice and collides with the other viewports'.
// `renderList` must target `orderingTable`.
constexpr std::int32_t orderingTablePosition(std::uint32_t renderList, std::uint32_t orderingTable, std::int32_t bin) {
  return static_cast<std::int32_t>((renderList - orderingTable) / sizeof(std::uint32_t)) + bin;
}

} // namespace crashbash::render
