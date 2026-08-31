#pragma once

#include <array>
#include <cstdint>

namespace crashbash::polar {

struct EffectRecipe {
  std::int16_t directionOffset;
  std::int16_t lateralOffset;
  std::int16_t forwardOffset;
  std::int16_t verticalOffset;
  std::int16_t sideOffset;
  std::int16_t choreographyOffset;
  std::int16_t sineMultiplier;
  std::uint8_t sineShift;
};

// Exact contact predicates shared by the native owner and its hermetic tests. Arithmetic is kept in
// MIPS-width terms: callers provide the low 32 bits of dx*dx + dz*dz, exactly as the retail body does.
bool isWithinContactDisk(std::int32_t deltaX, std::int32_t deltaZ, std::uint32_t distanceSquared);
std::int32_t contactThreshold(std::uint16_t configurationValue);
bool crossesContactThreshold(std::uint16_t configurationValue, std::int32_t motionSpeed, std::int16_t motionLimit);
const EffectRecipe &effectRecipe(std::uint32_t ordinal);

void registerPolarPushContactOverride();

} // namespace crashbash::polar
