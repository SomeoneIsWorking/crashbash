#pragma once

namespace crashbash {

// Native owners for the two BOOT-overlay object callbacks that sample libetc VSync. Registering them
// is what lets the product run past the frame at which the BOOT scene first drives an animated
// object; the retail bodies remain installed as their A/B supers.
void registerBootObjectCallbackOverrides();

} // namespace crashbash
