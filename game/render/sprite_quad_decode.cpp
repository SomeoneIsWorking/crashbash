#include "sprite_quad_decode.h"

#include <cstddef>

namespace crashbash::render {
namespace {

std::uint8_t fadedChannel(std::uint8_t channel, std::int32_t fade) {
  if (fade == 0) {
    return channel;
  }
  return static_cast<std::uint8_t>((static_cast<std::int32_t>(channel) * (0x1000 - fade)) >> 12);
}

} // namespace

std::optional<SpriteQuadDraw> decodeSpriteQuad(const SpriteQuadDescriptor &descriptor,
                                               const SpriteQuadCall &call,
                                               std::int16_t displayScale,
                                               std::int32_t fade) {
  if (displayScale == 0) {
    return std::nullopt;
  }

  const std::int32_t authoredX = static_cast<std::int16_t>(call.packedPosition);
  const std::int32_t authoredY = static_cast<std::int16_t>(call.packedPosition >> 16u);
  const std::int32_t x0 = authoredX * displayScale / 0x280;
  const std::int32_t y0 = authoredY / 2;
  const std::int32_t x1 = x0 + descriptor.width - 1;
  const std::int32_t y1 = y0 + descriptor.height - 1;
  const std::uint16_t texturePage =
      static_cast<std::uint16_t>((descriptor.texturePage & 0xFF9Fu) | ((call.colors[0] >> 19u) & 0x60u));

  SpriteQuadDraw draw{
      .sourceFunction = call.sourceFunction,
      .sourceAddress = call.descriptor,
      .renderList = call.renderList,
      .packedPosition = call.packedPosition,
      .orderingBin = call.orderingBin,
      .sourceColors = call.colors,
      .x = {x0, x1, x0, x1},
      .y = {y0, y0, y1, y1},
      .texturePage = texturePage,
      .clut = descriptor.clut,
      .blendMode = static_cast<std::uint8_t>((texturePage >> 5u) & 3u),
      .textured = true,
      .gouraud = call.gouraud,
      .dither = (texturePage & 0x0200u) != 0,
      .semiTransparent = static_cast<std::int32_t>(call.colors[0]) < 0,
  };
  for (std::size_t index = 0; index < draw.u.size(); ++index) {
    draw.u[index] = static_cast<std::uint8_t>(descriptor.textureCoordinates[index]);
    draw.v[index] = static_cast<std::uint8_t>(descriptor.textureCoordinates[index] >> 8u);
    draw.red[index] = fadedChannel(static_cast<std::uint8_t>(call.colors[index]), fade);
    draw.green[index] = fadedChannel(static_cast<std::uint8_t>(call.colors[index] >> 8u), fade);
    draw.blue[index] = fadedChannel(static_cast<std::uint8_t>(call.colors[index] >> 16u), fade);
  }
  return draw;
}

std::optional<SpriteQuadDraw> decodeScreenColorQuad(const ScreenColorQuadCall &call) {
  constexpr std::uint32_t kVisible = 0x00008000u;
  constexpr std::uint32_t kAuthoredScreen = 0x10000000u;
  if ((call.flags & kVisible) == 0 || (call.flags & kAuthoredScreen) == 0 || call.displayScale == 0) {
    return std::nullopt;
  }

  const std::uint32_t orderingBin = static_cast<std::uint32_t>(static_cast<std::int32_t>(call.depthBias)) >> 1u;
  if (orderingBin >= static_cast<std::uint32_t>(static_cast<std::int32_t>(call.depthLimit))) {
    return std::nullopt;
  }

  SpriteQuadDraw draw{
      .sourceFunction = call.sourceFunction,
      .sourceAddress = call.sourceAddress,
      .renderList = call.renderList,
      .orderingBin = static_cast<std::int32_t>(orderingBin),
      .sourceColors = call.colors,
      .blendMode = static_cast<std::uint8_t>((call.flags >> 5u) & 3u),
      .textured = false,
      .gouraud = true,
      .dither = true,
      .semiTransparent = (call.flags & 0x21u) != 0,
      .authoredWorldOrder = true,
  };
  for (std::size_t index = 0; index < draw.x.size(); ++index) {
    draw.x[index] = (call.xOffset + call.x[index]) * call.displayScale / 0x280;
    draw.y[index] = (call.yOffset + call.y[index]) / 2;
    draw.red[index] = fadedChannel(static_cast<std::uint8_t>(call.colors[index]), call.fade);
    draw.green[index] = fadedChannel(static_cast<std::uint8_t>(call.colors[index] >> 8u), call.fade);
    draw.blue[index] = fadedChannel(static_cast<std::uint8_t>(call.colors[index] >> 16u), call.fade);
  }
  return draw;
}

} // namespace crashbash::render
