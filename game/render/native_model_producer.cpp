#include "native_model_producer.h"

#include "core.h"
#include "game.h"
#include "model_face_coverage.h"
#include "model_face_pixel_diagnostic.h"
#include "model_material_diagnostic.h"
#include "native_projection.h"
#include "producer_scope.h"
#include "render_queue.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace crashbash::render {
namespace {

constexpr std::uint32_t kProducerKey = 0x80019F1Cu;

class RenderNodeScope {
public:
  RenderNodeScope(Core &core, std::uint32_t object) : core_(core), previous_(core.rsub.diag.currentNode()) {
    core_.rsub.diag.beginObject(object);
  }
  ~RenderNodeScope() {
    core_.rsub.diag.beginObject(previous_);
  }
  RenderNodeScope(const RenderNodeScope &) = delete;
  RenderNodeScope &operator=(const RenderNodeScope &) = delete;

private:
  Core &core_;
  std::uint32_t previous_;
};

} // namespace

NativeModelSubmitResult submitFixedModel(Core &core, const ModelDraw &draw) {
  if (!draw.transform.valid || draw.faces.empty() || core.game == nullptr || core.game->oracle ||
      core.rsub.mode.psxRender()) {
    return {};
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
    return {};
  }

  NativeModelSubmitResult result;
  int pixelProbeX = 0;
  int pixelProbeY = 0;
  const bool pixelProbeActive = core.game->gpu.pixel_probe_target(pixelProbeX, pixelProbeY);
  ProducerScope producer(&core.rsub.producerScope, kProducerKey, "model:fixed");
  RenderNodeScope renderNode(core, draw.object);
  for (const ModelFace &face : draw.faces) {
    std::array<psxport::native_projection::NativeProjectedVertex, 3> projected{};
    for (std::uint32_t i = 0; i < 3; ++i) {
      const ModelVertex &vertex = face.vertices[i];
      projected[i] =
          psxport::native_projection::project(affine, projection, {.x = vertex.x, .y = vertex.y, .z = vertex.z});
    }
    const std::array<ProjectedFaceVertex, 3> coverageVertices{{
        {.x = projected[0].sx, .y = projected[0].sy, .depth = projected[0].sz},
        {.x = projected[1].sx, .y = projected[1].sy, .depth = projected[1].sz},
        {.x = projected[2].sx, .y = projected[2].sy, .depth = projected[2].sz},
    }};
    const ModelFaceCoverage coverage = classifyFixedModelFace(
        coverageVertices, face.textured, face.vertices[2].flags, draw.depthBias, draw.depthLimit, draw.depthScale);
    const std::array<std::array<std::int32_t, 2>, 3> absoluteProjectedVertices{{
        {{projected[0].sx + gpu.s_off_x, projected[0].sy + gpu.s_off_y}},
        {{projected[1].sx + gpu.s_off_x, projected[1].sy + gpu.s_off_y}},
        {{projected[2].sx + gpu.s_off_x, projected[2].sy + gpu.s_off_y}},
    }};
    if (!coverage.accepted()) {
      if (pixelProbeActive) {
        observeModelFaceAtPixel(draw, face, absoluteProjectedVertices, coverage, false, pixelProbeX, pixelProbeY);
      }
      switch (coverage.rejection) {
      case ModelFaceRejection::ZeroUntexturedDepth:
        ++result.zeroDepthRejected;
        break;
      case ModelFaceRejection::FarDepth:
        ++result.farDepthRejected;
        break;
      case ModelFaceRejection::Winding:
        ++result.windingRejected;
        break;
      case ModelFaceRejection::None:
        break;
      }
      continue;
    }

    const std::array<std::array<std::int32_t, 2>, 3> projectedVertices{{
        {{projected[0].sx, projected[0].sy}},
        {{projected[1].sx, projected[1].sy}},
        {{projected[2].sx, projected[2].sy}},
    }};
    observeModelMaterialFace(draw, face, projectedVertices, coverage.sortKey);

    int xs[3]{}, ys[3]{}, us[3]{}, vs[3]{};
    float screenX[3]{}, screenY[3]{}, depth[3]{};
    unsigned char red[3]{}, green[3]{}, blue[3]{};
    // Retail bakes DPCS depth-cueing into the vertex colors it submits; face.colors is the raw
    // pre-cue source palette entry. The producer replays the retail-submitted colors.
    const std::array<std::uint32_t, 3> &renderColors = face.retailColors;
    for (std::uint32_t i = 0; i < 3; ++i) {
      xs[i] = projected[i].sx + gpu.s_off_x;
      ys[i] = projected[i].sy + gpu.s_off_y;
      // Retail rasterizes the quantized GTE SXY written into the packet. Feeding the pre-quantized
      // float projection to the native rasterizer changes edge coverage for thin faces even when
      // every packet SXY matches; Crashball's briefing arrows are a measured example. Native depth
      // remains continuous, but fixed-model screen coverage follows the verified packet contract.
      screenX[i] = static_cast<float>(xs[i]);
      screenY[i] = static_cast<float>(ys[i]);
      depth[i] = core.rsub.projParams.pzToOrd(projected[i].raw_view[2]);
      red[i] = static_cast<unsigned char>(renderColors[i]);
      green[i] = static_cast<unsigned char>(renderColors[i] >> 8u);
      blue[i] = static_cast<unsigned char>(renderColors[i] >> 16u);
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
    const int sortKey = static_cast<int>(coverage.sortKey);
    // The key's ord band: uniform over the FRAME-WIDE key domain (fixedModelSortKeyOrd). The key
    // itself is the authored order, so the carrier needs only to be injective with room for ties --
    // and it takes no per-draw term, or the same key would land in different bands per object.
    const float keyOrd = fixedModelSortKeyOrd(sortKey);
    const unsigned long long pushedBefore = queue.pushed_total;
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
                      sortKey,
                      keyOrd,
                      1,
                      dither);
    if (pixelProbeActive) {
      observeModelFaceAtPixel(draw,
                              face,
                              absoluteProjectedVertices,
                              coverage,
                              queue.pushed_total == pushedBefore + 1,
                              pixelProbeX,
                              pixelProbeY);
    }
    ++result.submitted;
  }
  return result;
}

void submitFixedModels(Core &core, SceneSnapshot &snapshot) {
  if (!snapshot.valid) {
    return;
  }
  for (ModelDraw &draw : snapshot.models) {
    const NativeModelSubmitResult submitted = submitFixedModel(core, draw);
    draw.nativeFacesSubmitted = submitted.submitted;
    draw.nativeZeroDepthRejected = submitted.zeroDepthRejected;
    draw.nativeFarDepthRejected = submitted.farDepthRejected;
    draw.nativeWindingRejected = submitted.windingRejected;
  }
}

} // namespace crashbash::render
