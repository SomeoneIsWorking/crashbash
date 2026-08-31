#pragma once

namespace crashbash {

// Adds the PC port's Start-or-Cross skip behavior to the BOOT logo controller. The handler schedules
// the same scene handoff the retail controller reaches naturally; it never changes logo phases or timers.
void registerBootLogoSkipOverride();

} // namespace crashbash
