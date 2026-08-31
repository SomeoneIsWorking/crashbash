#include "legacy_game_config.h"
#include "legacy_game_interface.h"

#include <cstdio>

int main() {
  if (crashbash::legacy::measuredConfig.preserveVramBackdrop != 1u) {
    std::fprintf(stderr,
                 "Crash Bash must declare guest VRAM as a visible picture producer while SCEA and mixed frames "
                 "still depend on it\n");
    return 1;
  }
  return 0;
}
