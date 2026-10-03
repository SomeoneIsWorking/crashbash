#pragma once

class Core;

namespace crashbash {

// The guest's LOADING card is loading-only presentation and never reaches the display. Its draw is
// `FUN_80018B08`, reached from the two handoff screens' own present handlers in BOOT.BIN; this
// owner retires the card at those two measured call sites and leaves every other caller — the
// menu screens draw their own panels through the same routine — on the guest's own body.
void registerLoadingCardSkipOverride(Core &core);

} // namespace crashbash
