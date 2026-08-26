#include "menu_boundary.h"

#include "core.h"
#include "override_registry.h"

#include <cstdint>
#include <mutex>

#include <lucent/log.h>

#ifdef CRASHBASH_HAVE_SUBSTRATE
#include "ov_menu_decls.h"
#endif

namespace crashbash::diagnostics {
namespace {

constexpr std::uint32_t kMenuEntry = 0x800B5244u;

#ifdef CRASHBASH_HAVE_SUBSTRATE
void observeMenuEntry(Core *core) {
  static std::once_flag marker;
  std::call_once(marker, [core] {
    lucent::info("crashbash-boundary", "MENU entry addr={:08X} ra={:08X}", kMenuEntry, core->r[31]);
  });
  ov_menu_gen_800B5244(core);
}
#endif

} // namespace

void registerMenuBoundary() {
#ifdef CRASHBASH_HAVE_SUBSTRATE
  overrides::install(
      kMenuEntry, "CrashBashDiagnostics::menuEntry", observeMenuEntry, ov_menu_gen_800B5244, ov_menu_set_override);
#else
  lucent::debug("crashbash-boundary", "MENU boundary registration deferred: no generated substrate");
#endif
}

} // namespace crashbash::diagnostics
