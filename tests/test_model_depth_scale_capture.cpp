#include "model_depth_scale_capture.h"

#include "game.h"

#include <cstdlib>
#include <memory>

int main() {
  auto game = std::make_unique<Game>();
  crashbash::render::publishModelDepthScales(game->core, 341u, 256u);
  if (game->core.rsub.projParams.zsf3() != 341 || game->core.rsub.projParams.zsf4() != 256) {
    return EXIT_FAILURE;
  }

  crashbash::render::publishModelDepthScales(game->core, 0xFFFFu, 0x8000u);
  if (game->core.rsub.projParams.zsf3() != -1 || game->core.rsub.projParams.zsf4() != -32768) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
