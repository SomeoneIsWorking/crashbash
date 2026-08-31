#pragma once

#include <cstdint>

namespace crashbash::render {

// DisplayFrame alternates two 0x1000-word ordering tables. A sprite producer records the selected
// bucket address inside one table, not that table's base address.
constexpr std::uint32_t kOrderingTableWordCount = 0x1000u;

constexpr bool spriteRenderListTargetsOrderingTable(std::uint32_t renderList, std::uint32_t orderingTable) {
  return renderList >= orderingTable && renderList - orderingTable < kOrderingTableWordCount * sizeof(std::uint32_t);
}

} // namespace crashbash::render
