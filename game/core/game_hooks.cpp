// The initial Crash Bash seam owns boot routing only; all game behavior remains on the substrate.
#include "core.h"
#include "game_iface.h"
#include <cstdlib>
#include <lucent/log.h>

static void boot_init(Core *core) {
  if (core->cfg->gameMain == 0) {
    lucent::error("boot", "Crash Bash guest main is unset; refusing to dispatch address zero");
    std::abort();
  }
  lucent::info("boot", "dispatching Crash Bash guest main 0x{:08X}", core->cfg->gameMain);
  rec_dispatch(core, core->cfg->gameMain);
}

static void register_overrides(Game *) {
  // No native game function is owned yet.  The empty hook is the truthful registration set.
}

static const GameHooks kCrashBashHooks = {
    .bootInit = boot_init,
    .registerOverrides = register_overrides,
};

const GameHooks *crashbash_game_hooks() {
  return &kCrashBashHooks;
}
