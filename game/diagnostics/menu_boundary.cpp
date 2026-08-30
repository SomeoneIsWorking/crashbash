#include "menu_boundary.h"

#include "core.h"
#include "override_registry.h"

#include <cstdint>
#include <mutex>

#include <lucent/log.h>

#ifdef CRASHBASH_HAVE_SUBSTRATE
#include "ov_dat28136_decls.h"
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
constexpr std::uint32_t kDat28136Registration = 0x800B4E1Cu;
constexpr std::uint32_t kDat28136Update = 0x800B4694u;
constexpr std::uint32_t kAppUpdateCallback = 0x8009F8B4u;

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

void observeDat28136Registration(Core *core) {
  const std::uint32_t returnAddress = core->r[31];
  const std::uint32_t callbackBefore = core->mem_r32(kAppUpdateCallback);
  ov_dat28136_gen_800B4E1C(core);
  lucent::info("crashbash-boundary",
               "DAT28136 registration addr={:08X} ra={:08X} callback={:08X}->{:08X}",
               kDat28136Registration,
               returnAddress,
               callbackBefore,
               core->mem_r32(kAppUpdateCallback));
}

void observeDat28136Update(Core *core) {
  static std::once_flag marker;
  std::call_once(marker, [core] {
    lucent::info("crashbash-boundary",
                 "DAT28136 update addr={:08X} ra={:08X} callback={:08X}",
                 kDat28136Update,
                 core->r[31],
                 core->mem_r32(kAppUpdateCallback));
  });
  ov_dat28136_gen_800B4694(core);
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
  overrides::install(kDat28136Registration,
                     "CrashBashDiagnostics::dat28136Registration",
                     observeDat28136Registration,
                     ov_dat28136_gen_800B4E1C,
                     ov_dat28136_set_override);
  overrides::install(kDat28136Update,
                     "CrashBashDiagnostics::dat28136Update",
                     observeDat28136Update,
                     ov_dat28136_gen_800B4694,
                     ov_dat28136_set_override);
#else
  lucent::debug("crashbash-boundary", "MENU boundary registration deferred: no generated substrate");
#endif
}

} // namespace crashbash::diagnostics
