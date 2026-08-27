#include "native_model_producer.h"

#include "core.h"
#include "game.h"
#include "native_projection.h"
#include "producer_scope.h"
#include "render_queue.h"

#include <array>
#include <cstdint>

namespace crashbash::render {
namespace {

constexpr std::uint32_t kProducerKey = 0x80019F1Cu;

bool frontFacing(const std::array<psxport::native_projection::NativeProjectedVertex, 3> &projected,
                 std::uint16_t flags) {
  if ((flags & 2u) != 0) {
    return true;
  }
  const std::int64_t area = static_cast<std::int64_t>(projected[0].sx) * projected[1].sy +
                            static_cast<std::int64_t>(projected[1].sx) * projected[2].sy +
                            static_cast<std::int64_t>(projected[2].sx) * projected[0].sy -
                            static_cast<std::int64_t>(projected[0].sx) * projected[2].sy -
                            static_cast<std::int64_t>(projected[1].sx) * projected[0].sy -
                            static_cast<std::int64_t>(projected[2].sx) * projected[1].sy;
  return (flags & 1u) == 0 ? area > 0 : area <= 0;
}

} // namespace

std::uint32_t submitFixedModel(Core &core, const ModelDraw &draw) {
  if (!draw.transform.valid || draw.faces.empty() || core.game == nullptr || core.game->oracle ||
      core.rsub.mode.psxRender()) {
    return 0;
  }
  psxport::native_projection::FixedAffine affine{
      .m = draw.transform.rotation,
      .t = draw.transform.translation,
  };
  const psxport::native_projection::ProjectionParams projection{
      .ofx = draw.transform.projectionX,
      .ofy = draw.transform.projectionY,
      .h = draw.transform.projectionDistance,
  };
  RenderQueue &queue = core.game->rq;
  const GpuState gpu = core.game->gpu;
  if (gpu.s_da_x0 > gpu.s_da_x1 || gpu.s_da_y0 > gpu.s_da_y1) {
    return 0;
  }

  std::uint32_t submitted = 0;
  ProducerScope producer(&core.rsub.producerScope, kProducerKey, "model:fixed");
  RenderQueue::PainterObjectScope painter(queue, draw.object);
  for (const ModelFace &face : draw.faces) {
    std::array<psxport::native_projection::NativeProjectedVertex, 3> projected{};
    bool visible = true;
    for (std::uint32_t i = 0; i < 3; ++i) {
      const ModelVertex &vertex = face.vertices[i];
      projected[i] =
          psxport::native_projection::project(affine, projection, {.x = vertex.x, .y = vertex.y, .z = vertex.z});
      visible = visible && projected[i].sz != 0;
    }
    if (!visible || !frontFacing(projected, face.vertices[2].flags)) {
      continue;
    }

    int xs[3]{}, ys[3]{}, us[3]{}, vs[3]{};
    float screenX[3]{}, screenY[3]{}, depth[3]{};
    unsigned char red[3]{}, green[3]{}, blue[3]{};
    for (std::uint32_t i = 0; i < 3; ++i) {
      xs[i] = projected[i].sx + gpu.s_off_x;
      ys[i] = projected[i].sy + gpu.s_off_y;
      screenX[i] = projected[i].px + static_cast<float>(gpu.s_off_x);
      screenY[i] = projected[i].py + static_cast<float>(gpu.s_off_y);
      depth[i] = core.rsub.projParams.pzToOrd(projected[i].raw_view[2]);
      red[i] = static_cast<unsigned char>(face.colors[i]);
      green[i] = static_cast<unsigned char>(face.colors[i] >> 8u);
      blue[i] = static_cast<unsigned char>(face.colors[i] >> 16u);
      us[i] = face.textureCoordinates[i] & 0xFFu;
      vs[i] = face.textureCoordinates[i] >> 8u;
    }
    const int textureMode = face.textured ? (face.texturePage >> 7u) & 3u : 3;
    const int texturePageX = face.textured ? (face.texturePage & 0x0Fu) * 64 : 0;
    const int texturePageY = face.textured ? ((face.texturePage >> 4u) & 1u) * 256 : 0;
    const int clutX = face.textured ? (face.clut & 0x3Fu) * 16 : 0;
    const int clutY = face.textured ? (face.clut >> 6u) & 0x1FFu : 0;
    const int blendMode = face.textured ? (face.texturePage >> 5u) & 3u : face.blendMode;
    const int dither = face.textured ? (face.texturePage >> 9u) & 1u : gpu.s_tp_dither;
    queue.emitOrQueue(&core,
                      1,
                      RQ_WORLD,
                      RQ_OM_DEPTH,
                      3,
                      face.semiTransparent ? 1 : 0,
                      0,
                      xs,
                      ys,
                      screenX,
                      screenY,
                      us,
                      vs,
                      red,
                      green,
                      blue,
                      depth,
                      textureMode,
                      texturePageX,
                      texturePageY,
                      clutX,
                      clutY,
                      gpu.s_tw_mx,
                      gpu.s_tw_my,
                      gpu.s_tw_ox,
                      gpu.s_tw_oy,
                      gpu.s_da_x0,
                      gpu.s_da_y0,
                      gpu.s_da_x1,
                      gpu.s_da_y1,
                      blendMode,
                      nullptr,
                      -1,
                      0.0f,
                      1,
                      dither);
    ++submitted;
  }
  return submitted;
}

} // namespace crashbash::render
