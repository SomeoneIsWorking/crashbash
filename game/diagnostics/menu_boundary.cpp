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
constexpr std::uint32_t kMenuUpdate = 0x800B3CA8u;
constexpr std::uint32_t kMenuAccept = 0x800B5360u;
constexpr std::uint32_t kInputEdge = 0x80051380u;
constexpr std::uint32_t kCurrentManager = 0x8009F8A4u;
constexpr std::uint32_t kPendingManager = 0x8009F8A8u;
constexpr std::uint32_t kStateIndex = 0x8005A648u;
constexpr std::uint32_t kSelection = 0x800B95F0u;

#ifdef CRASHBASH_HAVE_SUBSTRATE
void observeMenuEntry(Core *core) {
  static std::once_flag marker;
  std::call_once(marker, [core] {
    lucent::info("crashbash-boundary", "MENU entry addr={:08X} ra={:08X}", kMenuEntry, core->r[31]);
  });
  ov_menu_gen_800B5244(core);
}

void observeMenuUpdate(Core *core) {
  const std::uint32_t edge = core->mem_r32(kInputEdge);
  lucent::debug("crashbash-boundary",
                "MENU input edge={:08X} current={:08X} pending={:08X} state-index={:08X} selection={:08X}",
                edge,
                core->mem_r32(kCurrentManager),
                core->mem_r32(kPendingManager),
                core->mem_r32(kStateIndex),
                core->mem_r32(kSelection));
  ov_menu_gen_800B3CA8(core);
}

void observeMenuAccept(Core *core) {
  const std::uint32_t edge = core->mem_r32(kInputEdge);
  const std::uint32_t current = core->mem_r32(kCurrentManager);
  const std::uint32_t pendingBefore = core->mem_r32(kPendingManager);
  const std::uint32_t selection = core->mem_r32(kSelection);
  ov_menu_gen_800B5360(core);
  lucent::info("crashbash-boundary",
               "MENU accept edge={:08X} current={:08X} pending={:08X}->{:08X} selection={:08X}",
               edge,
               current,
               pendingBefore,
               core->mem_r32(kPendingManager),
               selection);
}
#endif

} // namespace

void registerMenuBoundary() {
#ifdef CRASHBASH_HAVE_SUBSTRATE
  overrides::install(
      kMenuEntry, "CrashBashDiagnostics::menuEntry", observeMenuEntry, ov_menu_gen_800B5244, ov_menu_set_override);
  overrides::install(
      kMenuUpdate, "CrashBashDiagnostics::menuUpdate", observeMenuUpdate, ov_menu_gen_800B3CA8, ov_menu_set_override);
  overrides::install(
      kMenuAccept, "CrashBashDiagnostics::menuAccept", observeMenuAccept, ov_menu_gen_800B5360, ov_menu_set_override);
#else
  lucent::debug("crashbash-boundary", "MENU boundary registration deferred: no generated substrate");
#endif
}

} // namespace crashbash::diagnostics
