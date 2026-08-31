#include "sprite_render_list.h"

#include <cstdlib>

int main() {
  using crashbash::render::spriteRenderListTargetsOrderingTable;

  constexpr unsigned kTable = 0x8005F79Cu;
  if (!spriteRenderListTargetsOrderingTable(kTable, kTable) ||
      !spriteRenderListTargetsOrderingTable(0x8006179Cu, kTable) ||
      spriteRenderListTargetsOrderingTable(0x8005B790u, kTable) ||
      spriteRenderListTargetsOrderingTable(0x8006379Cu, kTable)) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
