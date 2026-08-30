#include "native_sprite_quad_producer.h"

#include "core.h"
#include "game.h"
#include "model_face_coverage.h"
#include "producer_scope.h"
#include "render_queue.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace crashbash::render {
namespace {

constexpr std::uint32_t kGouraudSpriteQuadSubmit = 0x8002992Cu;
constexpr std::uint32_t kFlatSpriteQuadSubmit = 0x80029D28u;
constexpr std::uint32_t kScreenColorQuadSubmit = 0x8001A0D8u;

const char *producerName(std::uint32_t sourceFunction) {
  if (sourceFunction == kFlatSpriteQuadSubmit) {
    return "sprite:ft4";
  }
  return sourceFunction == kScreenColorQuadSubmit ? "hud:g4" : "sprite:gt4";
}

void submitSpriteQuad(Core &core, const SpriteQuadDraw &draw) {
  if (core.game == nullptr || core.game->oracle || core.rsub.mode.psxRender()) {
    return;
  }
  const GpuState gpu = core.game->gpu;
  if ((draw.textured && (draw.x[0] > draw.x[1] || draw.y[0] > draw.y[2])) || gpu.s_da_x0 > gpu.s_da_x1 ||
      gpu.s_da_y0 > gpu.s_da_y1) {
    return;
  }

  int xs[4]{};
  int ys[4]{};
  int us[4]{};
  int vs[4]{};
  unsigned char red[4]{};
  unsigned char green[4]{};
  unsigned char blue[4]{};
  for (std::size_t index = 0; index < draw.x.size(); ++index) {
    xs[index] = draw.x[index] + gpu.s_off_x;
    ys[index] = draw.y[index] + gpu.s_off_y;
    us[index] = draw.u[index];
    vs[index] = draw.v[index];
    red[index] = draw.red[index];
    green[index] = draw.green[index];
    blue[index] = draw.blue[index];
  }

  RenderQueue &queue = core.game->rq;
  const int layer = draw.authoredWorldOrder ? RQ_WORLD : RQ_HUD;
  const int orderMode = draw.authoredWorldOrder ? RQ_OM_DEPTH : RQ_OM_2D_FG;
  const int sortKey = draw.authoredWorldOrder ? draw.orderingBin : -1;
  const float keyOrd = draw.authoredWorldOrder ? fixedModelSortKeyOrd(sortKey) : 0.0f;
  const float depth[4] = {keyOrd, keyOrd, keyOrd, keyOrd};
  const int textureMode = draw.textured ? (draw.texturePage >> 7u) & 3u : 3;
  const int texturePageX = draw.textured ? (draw.texturePage & 0x0Fu) * 64 : 0;
  const int texturePageY = draw.textured ? ((draw.texturePage >> 4u) & 1u) * 256 : 0;
  const int clutX = draw.textured ? (draw.clut & 0x3Fu) * 16 : 0;
  const int clutY = draw.textured ? (draw.clut >> 6u) & 0x1FFu : 0;
  queue.emitOrQueue(&core,
                    1,
                    layer,
                    orderMode,
                    4,
                    draw.semiTransparent ? 1 : 0,
                    0,
                    xs,
                    ys,
                    draw.authoredWorldOrder ? depth : nullptr,
                    nullptr,
                    us,
                    vs,
                    red,
                    green,
                    blue,
                    nullptr,
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
                    draw.blendMode,
                    nullptr,
                    sortKey,
                    keyOrd,
                    draw.gouraud ? 1 : 0,
                    draw.dither ? 1 : 0);
}

} // namespace

void submitSpriteQuads(Core &core, const SceneSnapshot &snapshot, std::uint32_t renderList) {
  if (!snapshot.valid || snapshot.spriteQuads.empty() || core.game == nullptr || core.game->oracle ||
      core.rsub.mode.psxRender()) {
    return;
  }

  std::vector<std::size_t> order;
  order.reserve(snapshot.spriteQuads.size());
  for (std::size_t index = 0; index < snapshot.spriteQuads.size(); ++index) {
    if (snapshot.spriteQuads[index].renderList == renderList) {
      order.push_back(index);
    }
  }
  if (order.empty()) {
    return;
  }
  std::sort(order.begin(), order.end(), [&snapshot](std::size_t left, std::size_t right) {
    const SpriteQuadDraw &a = snapshot.spriteQuads[left];
    const SpriteQuadDraw &b = snapshot.spriteQuads[right];
    if (a.orderingBin != b.orderingBin) {
      return a.orderingBin > b.orderingBin;
    }
    return left > right;
  });

  for (const std::size_t index : order) {
    const SpriteQuadDraw &draw = snapshot.spriteQuads[index];
    if (draw.sourceFunction != kGouraudSpriteQuadSubmit && draw.sourceFunction != kFlatSpriteQuadSubmit &&
        draw.sourceFunction != kScreenColorQuadSubmit) {
      continue;
    }
    ProducerScope producer(&core.rsub.producerScope, draw.sourceFunction, producerName(draw.sourceFunction));
    submitSpriteQuad(core, draw);
  }
}

} // namespace crashbash::render
