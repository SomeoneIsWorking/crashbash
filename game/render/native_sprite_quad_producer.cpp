#include "native_sprite_quad_producer.h"

#include "core.h"
#include "game.h"
#include "gpu_vk.h"
#include "hud_layout.h"
#include "model_face_coverage.h"
#include "producer_scope.h"
#include "render_queue.h"
#include "sprite_render_list.h"
#include "ui_anchor.h"

#include <lucent/log.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
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

// The horizontal shift this element needs BEFORE the framework's own 2D layout transform runs.
//
// The framework centres every authored 4:3 2D x by the margin (psxport `RQ_2D_AUTHORED_4_3`), which
// is the right answer for a centred element and the wrong one for a corner panel: it slides the
// panel off the edge it belongs to and into the arena the widening revealed. So the class is read
// from the element's own authored rectangle — see hud_layout.h — and what the producer applies is
// the DIFFERENCE between that class and the centring the framework is about to do. At 4:3 the
// margin is zero and this is the identity, which is why the 4:3 picture cannot change.
//
// The world-ordered screen-color quads are NOT here: they already carry the authored canvas shift
// below and are submitted with `RQ_OM_DEPTH`, which never enters the 2D transform at all.
int horizontalCorrection(Core &core,
                         const SpriteQuadDraw &draw,
                         std::uint32_t logicFrame,
                         bool authoredScreenPresentation) {
  if (draw.authoredWorldOrder) {
    return 0;
  }
  const std::optional<hud_layout::Caller> caller = hud_layout::callerOf(draw.callerReturnAddress);
  if (!caller) {
    // An element this port has not classified stays exactly where the framework's own rule puts it.
    // Guessing a class for an unknown element is how a HUD ends up half-migrated at 16:9, so the
    // unknown is reported and the framework's answer stands.
    lucent::warn("uihud",
                 "UNCLASSIFIED element caller={:08X} source={:08X} authored=({}, {}) — left on the "
                 "framework's centring rule",
                 draw.callerReturnAddress,
                 draw.sourceFunction,
                 draw.x[0],
                 draw.x[1]);
    return 0;
  }
  const ui_anchor::Frame frame = ui_anchor::frame(&core);
  const std::int32_t width = draw.x[1] - draw.x[0] + 1;
  const ui_anchor::Anchor anchor = hud_layout::anchorFor(*caller, draw.x[0], width, frame, authoredScreenPresentation);
  return ui_anchor::correctionAndReport(
      logicFrame, hud_layout::forCaller(*caller)->name, anchor, draw.x[0], width, frame);
}

void submitSpriteQuad(Core &core,
                      const SpriteQuadDraw &draw,
                      std::uint32_t logicFrame,
                      bool authoredScreenPresentation) {
  if (core.game == nullptr || core.rsub.mode.psxRender()) {
    return;
  }
  const GpuState gpu = core.game->gpu;
  if ((draw.textured && (draw.x[0] > draw.x[1] || draw.y[0] > draw.y[2])) || gpu.s_da_x0 > gpu.s_da_x1 ||
      gpu.s_da_y0 > gpu.s_da_y1) {
    return;
  }
  const int authoredCanvasShift =
      draw.authoredWorldOrder && !draw.textured && gpu_vk_wide_engine(&core) ? gpu_vk_wide_left_margin(&core) : 0;
  int drawAreaX0 = gpu.s_da_x0;
  int drawAreaX1 = gpu.s_da_x1;
  if (authoredScreenPresentation && gpu_vk_wide_engine(&core)) {
    const int margin = gpu_vk_wide_left_margin(&core);
    drawAreaX0 = std::max(drawAreaX0, margin);
    drawAreaX1 = std::min(drawAreaX1, margin + gpu_vk_native_w(&core) - 1);
  }

  const int anchoredShift = horizontalCorrection(core, draw, logicFrame, authoredScreenPresentation);
  // THE CLIP TRAVELS WITH THE ELEMENT. The framework's 2D transform shifts the submitted draw area by
  // the same margin it shifts the vertices, and this element's vertices were just moved BACK by the
  // margin the anchoring applied. Leaving the clip behind clips the element against a rectangle that
  // no longer bounds it: at 16:9 a left-edge panel authored at x = 32 was cut at x = 86, so two
  // thirds of the corner panel the widening was supposed to reveal never reached the screen. Both
  // numbers are in the element's own authored space, so they are corrected together.
  drawAreaX0 += anchoredShift;
  drawAreaX1 += anchoredShift;
  int xs[4]{};
  int ys[4]{};
  int us[4]{};
  int vs[4]{};
  unsigned char red[4]{};
  unsigned char green[4]{};
  unsigned char blue[4]{};
  for (std::size_t index = 0; index < draw.x.size(); ++index) {
    xs[index] = draw.x[index] + anchoredShift + authoredCanvasShift + gpu.s_off_x;
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
                    drawAreaX0,
                    gpu.s_da_y0,
                    drawAreaX1,
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
  if (!snapshot.valid || snapshot.spriteQuads.empty() || core.game == nullptr || core.rsub.mode.psxRender()) {
    return;
  }

  std::vector<std::size_t> order;
  order.reserve(snapshot.spriteQuads.size());
  for (std::size_t index = 0; index < snapshot.spriteQuads.size(); ++index) {
    if (spriteRenderListTargetsOrderingTable(snapshot.spriteQuads[index].renderList, renderList)) {
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
    submitSpriteQuad(core, draw, snapshot.logicFrame, snapshot.authoredScreenPresentation);
  }
}

} // namespace crashbash::render
