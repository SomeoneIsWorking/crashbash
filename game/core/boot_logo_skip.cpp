#include "boot_logo_skip.h"

#include "core.h"
#include "crashbash_guest.h"
#include "game.h"
#include "measured_guest_call.h"
#include "override_registry.h"

#include <cstdint>

#include <lucent/log.h>

#ifdef CRASHBASH_HAVE_SUBSTRATE
#include "ov_boot_decls.h"
#include "rec_decls.h"
#endif

namespace crashbash {
namespace {

#ifdef CRASHBASH_HAVE_SUBSTRATE
constexpr std::uint16_t kStartButton = 0x0008u;
constexpr std::uint16_t kCrossButton = 0x4000u;
constexpr std::uint16_t kSkipButtons = kStartButton | kCrossButton;
constexpr std::uint32_t kScenePendingState = guest::kSceneTransition + 4u;

void bootLogoUpdateOwned(Core *core) {
  // The logo controller's natural completion writes this same target/flag pair and returns to the
  // scene dispatcher. Request it through 0x8001E588 instead: on the next dispatch pass it invokes
  // the outgoing scene's exit callback, then enters the measured handoff state. This deliberately
  // leaves the logo controller's phase, wait counter, objects, and resource ownership untouched.
  if (core->game->pad.pressedButton(kSkipButtons) && core->mem_r32(kScenePendingState) == 0u) {
    measuredGuestCall(*core,
                      guest::kSceneTransitionRequest,
                      0x80092B28u,
                      3u,
                      guest::kSceneTransition,
                      guest::kBootLogoHandoffState,
                      guest::kBootLogoHandoffFlags);
    lucent::info("boot", "Start or Cross requested the BOOT logo handoff through the scene lifecycle");
    return;
  }

  ov_boot_gen_8008E5BC(core);
}
#endif

} // namespace

void registerBootLogoSkipOverride() {
#ifdef CRASHBASH_HAVE_SUBSTRATE
  overrides::install(guest::kBootLogoUpdate,
                     "CrashBash::BootLogoSkip",
                     bootLogoUpdateOwned,
                     ov_boot_gen_8008E5BC,
                     ov_boot_set_override);
#else
  lucent::debug("boot", "BOOT logo Start-skip registration deferred: no generated substrate");
#endif
}

} // namespace crashbash
