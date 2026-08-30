#include "sprite_quad_decode.h"

#include <cstdlib>

int main() {
  using crashbash::render::decodeSpriteQuad;
  using crashbash::render::SpriteQuadCall;
  using crashbash::render::SpriteQuadDescriptor;

  const SpriteQuadDescriptor descriptor{
      .width = 16,
      .height = 9,
      .texturePage = 0x038Fu,
      .clut = 0x4567u,
      .textureCoordinates = {0x2010u, 0x2030u, 0x4010u, 0x4030u},
  };
  const SpriteQuadCall call{
      .sourceFunction = 0x8002992Cu,
      .descriptor = 0x800A1000u,
      .renderList = 0x8005F79Cu,
      .packedPosition = 0xFFFBFFECu,
      .orderingBin = 3,
      .colors = {0x83180080u, 0x00004020u, 0x00FF8040u, 0x00100804u},
      .gouraud = true,
  };
  const auto draw = decodeSpriteQuad(descriptor, call, 0x280, 0x800);
  if (!draw || draw->sourceFunction != call.sourceFunction || draw->descriptor != call.descriptor ||
      draw->renderList != call.renderList || draw->orderingBin != 3 ||
      draw->x != std::array<std::int32_t, 4>{-20, -5, -20, -5} ||
      draw->y != std::array<std::int32_t, 4>{-2, -2, 6, 6} ||
      draw->u != std::array<std::uint8_t, 4>{0x10, 0x30, 0x10, 0x30} ||
      draw->v != std::array<std::uint8_t, 4>{0x20, 0x20, 0x40, 0x40} ||
      draw->red != std::array<std::uint8_t, 4>{0x40, 0x10, 0x20, 0x02} ||
      draw->green != std::array<std::uint8_t, 4>{0x00, 0x20, 0x40, 0x04} ||
      draw->blue != std::array<std::uint8_t, 4>{0x0C, 0x00, 0x7F, 0x08} || draw->texturePage != 0x03EFu ||
      draw->clut != descriptor.clut || !draw->gouraud || !draw->semiTransparent) {
    return EXIT_FAILURE;
  }
  SpriteQuadCall flatCall = call;
  flatCall.sourceFunction = 0x80029D28u;
  flatCall.colors.fill(0x00402010u);
  flatCall.gouraud = false;
  const auto flatDraw = decodeSpriteQuad(descriptor, flatCall, 0x280, 0);
  if (!flatDraw || flatDraw->sourceFunction != flatCall.sourceFunction || flatDraw->gouraud ||
      flatDraw->red != std::array<std::uint8_t, 4>{0x10, 0x10, 0x10, 0x10} ||
      flatDraw->green != std::array<std::uint8_t, 4>{0x20, 0x20, 0x20, 0x20} ||
      flatDraw->blue != std::array<std::uint8_t, 4>{0x40, 0x40, 0x40, 0x40}) {
    return EXIT_FAILURE;
  }
  if (decodeSpriteQuad(descriptor, call, 0, 0)) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
