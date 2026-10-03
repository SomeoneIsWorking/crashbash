#pragma once

#include <array>
#include <cstdint>

class Core;

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

// The running totals of the Polar Push contact traversal. Diagnostic only: the traversal's decisions
// are made from guest state, never from these numbers. One instance belongs to the Core whose run it
// measures, so two Cores cannot share a total.
struct ContactCensus {
  std::uint64_t invocations = 0;
  std::uint64_t scanned = 0;
  std::uint64_t active = 0;
  std::uint64_t diskPass = 0;
  std::uint64_t retained = 0;
  std::uint64_t consumed = 0;
  std::uint64_t emittedEffects = 0;
};

void registerPolarPushContactOverride(Core &core);

} // namespace crashbash::polar
