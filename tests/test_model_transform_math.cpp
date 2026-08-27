#include "model_transform_capture.h"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
  constexpr crashbash::render::ModelRotation identity{{{0x1000, 0, 0}, {0, 0x1000, 0}, {0, 0, 0x1000}}};
  constexpr crashbash::render::ModelRotation rotation{{{0, -0x1000, 0}, {0x1000, 0, 0}, {0, 0, 0x1000}}};
  assert(crashbash::render::composeModelRotations(identity, rotation) == rotation);
  assert(crashbash::render::composeModelRotations(rotation, identity) == rotation);

  constexpr std::array<std::int32_t, 3> translation{0x18001, -0x18001, 0x7FFF};
  assert(crashbash::render::transformLargeModelTranslation(identity, translation) == translation);

  constexpr crashbash::render::ModelRotation saturated{{{0x7FFF, 0x7FFF, 0x7FFF}, {0, 0, 0}, {0, 0, 0}}};
  const std::array<std::int32_t, 3> saturatedResult =
      crashbash::render::transformLargeModelTranslation(saturated, {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF});
  assert(saturatedResult[0] == -0x38001);
  return 0;
}
