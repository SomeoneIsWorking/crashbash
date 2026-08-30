#pragma once

#include "scene_snapshot.h"

#include <array>
#include <cstdint>
#include <optional>

namespace crashbash::render {

struct SpriteQuadDescriptor {
  std::uint16_t width = 0;
  std::uint16_t height = 0;
  std::uint16_t texturePage = 0;
  std::uint16_t clut = 0;
  std::array<std::uint16_t, 4> textureCoordinates{};
};

struct SpriteQuadCall {
  std::uint32_t sourceFunction = 0;
  std::uint32_t descriptor = 0;
  std::uint32_t renderList = 0;
  std::uint32_t packedPosition = 0;
  std::int32_t orderingBin = 0;
  std::array<std::uint32_t, 4> colors{};
  bool gouraud = false;
};

// Decode only the game-authored inputs consumed by the 0x8002992C Gouraud and 0x80029D28 flat-color
// textured-quad leaves. A zero display scale is not a valid recipe: both retail bodies trap on the
// same divisor, so callers retain the super and decline native submission instead of manufacturing
// geometry.
std::optional<SpriteQuadDraw> decodeSpriteQuad(const SpriteQuadDescriptor &descriptor,
                                               const SpriteQuadCall &call,
                                               std::int16_t displayScale,
                                               std::int32_t fade);

} // namespace crashbash::render
