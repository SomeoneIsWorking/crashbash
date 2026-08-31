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

struct ScreenColorQuadCall {
  std::uint32_t sourceFunction = 0;
  std::uint32_t sourceAddress = 0;
  std::uint32_t renderList = 0;
  std::uint32_t flags = 0;
  std::array<std::int16_t, 4> x{};
  std::array<std::int16_t, 4> y{};
  std::array<std::uint32_t, 4> colors{};
  std::int32_t xOffset = 0;
  std::int32_t yOffset = 0;
  std::int16_t displayScale = 0;
  std::int16_t depthBias = 0;
  std::int16_t depthLimit = 0;
  std::int32_t fade = 0;
};

// A large source-authored color quad identifies a complete briefing composition. Smaller screen
// quads are menu chrome over a 3D scene and must leave the widened world enabled.
inline bool isCentered4x3Composition(int width, int height, int nativeWidth, int nativeHeight) {
  return width > 0 && height > 0 && nativeWidth > 0 && nativeHeight > 0 && width * 4 >= nativeWidth * 3 &&
         height * 2 >= nativeHeight;
}

// Decode only the game-authored inputs consumed by the 0x8002992C Gouraud and 0x80029D28 flat-color
// textured-quad leaves. A zero display scale is not a valid recipe: both retail bodies trap on the
// same divisor, so callers retain the super and decline native submission instead of manufacturing
// geometry.
std::optional<SpriteQuadDraw> decodeSpriteQuad(const SpriteQuadDescriptor &descriptor,
                                               const SpriteQuadCall &call,
                                               std::int16_t displayScale,
                                               std::int32_t fade);

// Decode the source-authored screen branch of 0x8001A0D8. GTE-mode calls are deliberately declined:
// they require a separately proven pre-projection transform recipe.
std::optional<SpriteQuadDraw> decodeScreenColorQuad(const ScreenColorQuadCall &call);

} // namespace crashbash::render
