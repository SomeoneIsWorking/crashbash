#include "crashbash_application.h"

#include "core.h"
#include "crashbash_runtime.h"
#include "game.h"

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

namespace crashbash {

int runApplication(const std::filesystem::path &executablePath) {
  // Installation precedes the first Core, which snapshots this process-lifetime derived owner.
  static CrashBashRuntime runtime;
  psxport_install_game(runtime);
  crashbash_install_recomp();

  if (!std::filesystem::is_regular_file(executablePath)) {
    lucent::error("boot", "{} is absent; provision the verified retail executable first", executablePath.string());
    return 2;
  }

  auto game = std::make_unique<Game>();
  Core *core = &game->core;
  watchdog_init();
  load_exe(executablePath.c_str(), core);
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

} // namespace crashbash
