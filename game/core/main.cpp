// Crash Bash process entry: install the measured seam, load the retail image, and enter guest main.
#include "core.h"
#include "crashbash_runtime.h"
#include "game.h"
#include <filesystem>
#include <lucent/log.h>
#include <memory>

extern "C" {
void mdec_init();
void spu_init();
void watchdog_init();
}

void gte_init();
void load_exe(const char *path, Core *core);
void native_boot_run(Core *core);
void crashbash_install_recomp();

static constexpr const char *kDefaultExecutable = "scratch/bin/crashbash/SCUS_945.70";

int main(int argc, char **argv) {
  // Installation precedes the first Core, which snapshots this process-lifetime derived owner.
  static crashbash::CrashBashRuntime runtime;
  psxport_install_game(runtime);
  crashbash_install_recomp();

  const char *path = argc > 1 ? argv[1] : kDefaultExecutable;
  if (!std::filesystem::is_regular_file(path)) {
    lucent::error("boot", "{} is absent; provision the verified retail executable first", path);
    return 2;
  }

  auto game = std::make_unique<Game>();
  Core *core = &game->core;
  watchdog_init();
  load_exe(path, core);
  gte_init();
  mdec_init();
  spu_init();
  game->spu_audio.init();
  game->gpu.gpu_native_init();
  game->cd.overridesInit();
  game->platform_hle.initBuiltins();
  game->pad.overridesInit();
  core->r[4] = 1;
  core->r[5] = 0;
  core->runtime->registerOverrides(*game);
  native_boot_run(core);
  lucent::info("boot", "Crash Bash native boot returned");
  return 0;
}
