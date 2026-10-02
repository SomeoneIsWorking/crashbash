#include "sprite_render_list.h"

#include <cstdlib>

int main() {
  using crashbash::render::orderingTablePosition;
  using crashbash::render::spriteRenderListTargetsOrderingTable;

  constexpr unsigned kTable = 0x8005F79Cu;
  if (!spriteRenderListTargetsOrderingTable(kTable, kTable) ||
      !spriteRenderListTargetsOrderingTable(0x8006179Cu, kTable) ||
      spriteRenderListTargetsOrderingTable(0x8005B790u, kTable) ||
      spriteRenderListTargetsOrderingTable(0x8006379Cu, kTable)) {
    return EXIT_FAILURE;
  }
  // CHOOSE LEVEL: the panels' screen quads sit at bin 128 of the backdrop viewport's slice 2048; the
  // arena preview behind them inserts into slice 512. The bins alone would put the preview's far half
  // behind the panel; the table words put the whole preview in front, as retail draws it.
  if (orderingTablePosition(kTable + 2048u * 4u, kTable, 128) != 2176 ||
      orderingTablePosition(kTable + 512u * 4u, kTable, 300) != 812 || orderingTablePosition(kTable, kTable, 7) != 7) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
