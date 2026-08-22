#include "crashbash_runtime.h"

#include "core.h"
#include "legacy_game_interface.h"

#include <cstdlib>

#include <lucent/log.h>

namespace crashbash {

CrashBashRuntime::CrashBashRuntime() : LegacyGameRuntimeAdapter(legacy::measuredConfig, legacy::compatibilityHooks) {}

void CrashBashRuntime::registerOverrides(Game &) {
  // No native game function is owned yet. The empty override set preserves guest execution.
}

void CrashBashRuntime::bootInit(Core &core) {
  const GameConfig *config = legacyConfigForMigration();
  if (!config || config->gameMain == 0) {
    lucent::error("boot", "Crash Bash guest main is unset; refusing to dispatch address zero");
    std::abort();
  }
  lucent::info("boot", "dispatching Crash Bash guest main 0x{:08X}", config->gameMain);
  rec_dispatch(&core, config->gameMain);
}

} // namespace crashbash
