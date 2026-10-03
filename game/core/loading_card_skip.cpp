#include "loading_card_skip.h"

#include "core.h"
#include "crashbash_guest.h"
#include "guest_execution.h"

#include <cstdint>

namespace crashbash {
namespace {

// The two call sites that draw the card, measured in BOOT.BIN and keyed on the return address the
// guest leaves in r[31]: `jal 0x80018B08` at 0x80094240 (screen 0x8009F998, the general handoff)
// and at 0x80094180 (screen 0x8009FA00, the level-0x28 route). Both presents run only while the
// handoff scene 0x800A00DC is current.
constexpr std::uint32_t kCardDrawReturnAddress = 0x80094248u;    // BOOT FUN_8009421C, screen 0x8009F998
constexpr std::uint32_t kLevelScreenReturnAddress = 0x80094188u; // BOOT FUN_8009414C, screen 0x8009FA00

void loadingCardDrawOwned(Core *core) {
  // Both callers are the handoff screen presents, whose other work is unchanged: BOOT's own
  // FUN_8009421C still publishes the transition clock into the fade word and presents the level the
  // handoff has built, and FUN_8009414C still presents its own level route. Only the card — the
  // animated LOADING panel, which exists to cover a wait this port does not have — is retired, so
  // the handoff presents the level the guest built and, before it has one, holds the last real
  // picture the way the frame driver already holds an unpresented frame.
  if (core->r[31] == kCardDrawReturnAddress || core->r[31] == kLevelScreenReturnAddress) {
    return;
  }
  runtime::callOriginal(*core, runtime::GuestImage::Resident, guest::kLoadingCardDraw);
}

} // namespace

void registerLoadingCardSkipOverride(Core &core) {
  runtime::registerNativeOverride(
      core, runtime::GuestImage::Resident, guest::kLoadingCardDraw, "CrashBash::LoadingCardSkip", loadingCardDrawOwned);
}

} // namespace crashbash
