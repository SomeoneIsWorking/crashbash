#include "boot_logo_skip.h"

#include "core.h"
#include "crashbash_guest.h"
#include "game.h"
#include "guest_execution.h"
#include "measured_guest_call.h"

#include <cstdint>

#include <lucent/log.h>

namespace crashbash {
namespace {

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

  runtime::callOriginal(*core, runtime::GuestImage::Boot, guest::kBootLogoUpdate);
}

} // namespace

void registerBootLogoSkipOverride(Core &core) {
  runtime::registerNativeOverride(
      core, runtime::GuestImage::Boot, guest::kBootLogoUpdate, "CrashBash::BootLogoSkip", bootLogoUpdateOwned);
}

} // namespace crashbash
