#include "native_model_producer.h"

#include "core.h"
#include "game.h"
#include "gpu_vk.h"
#include "interpolated_scene.h"
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

ModelRenderEnvironment captureRenderEnvironment(const Core &core) {
  const GpuState &gpu = core.game->gpu;
  return {
      .drawOffsetX = gpu.s_off_x,
      .drawOffsetY = gpu.s_off_y,
      .drawAreaX0 = gpu.s_da_x0,
      .drawAreaY0 = gpu.s_da_y0,
      .drawAreaX1 = gpu.s_da_x1,
      .drawAreaY1 = gpu.s_da_y1,
      .textureWindowMaskX = gpu.s_tw_mx,
      .textureWindowMaskY = gpu.s_tw_my,
      .textureWindowOffsetX = gpu.s_tw_ox,
      .textureWindowOffsetY = gpu.s_tw_oy,
      .textureDither = gpu.s_tp_dither,
      .valid = true,
  };
}

} // namespace

NativeModelSubmitResult submitFixedModel(Core &core,
                                         const ModelDraw &draw,
                                         const ModelDraw *previous,
                                         float alpha,
                                         const ModelRenderEnvironment &environment) {
  if (!draw.transform.valid || draw.faces.empty() || core.game == nullptr || core.game->oracle ||
      core.rsub.mode.psxRender() || !environment.valid) {
    return {};
  }
  const ModelTransform transform =
      previous ? interpolateModelTransform(previous->transform, draw.transform, alpha) : draw.transform;
  psxport::native_projection::FixedAffine affine{
      .m = transform.rotation,
      .t = transform.translation,
  };
  psxport::native_projection::ProjectionParams projection{
      .ofx = transform.projectionX,
      .ofy = transform.projectionY,
      .h = transform.projectionDistance,
  };
  std::int32_t drawAreaX0 = environment.drawAreaX0;
  std::int32_t drawAreaX1 = environment.drawAreaX1;
  if (gpu_vk_wide_engine(&core)) {
    // The captured title camera is authored in the native 4:3 framebuffer. Widening adds equal
    // columns around that frame, so preserve any title viewport offset and move its projection
    // origin by exactly the new left margin. Replacing OFX with the framebuffer midpoint would
    // destroy split/offset viewport intent; leaving it unchanged left-aligns the whole arena.
    const std::int32_t margin = gpu_vk_wide_engine_ofx(&core) - gpu_vk_native_w(&core) / 2;
    projection.ofx += margin << 16;
    if (environment.authoredScreenPresentation) {
      // The briefing is one 4:3 composition. Its world, dimmer, border, text, and HUD must share
      // the same centred viewport rather than exposing extra arena columns behind a fixed panel.
      drawAreaX0 = std::max(drawAreaX0, margin);
      drawAreaX1 = std::min(drawAreaX1, margin + gpu_vk_native_w(&core) - 1);
    }
  }
  RenderQueue &queue = core.game->activeRq();
  if (drawAreaX0 > drawAreaX1 || environment.drawAreaY0 > environment.drawAreaY1) {
    return {};
  }

  NativeModelSubmitResult result;
  int pixelProbeX = 0;
  int pixelProbeY = 0;
  const bool pixelProbeActive = core.game->gpu.pixel_probe_target(pixelProbeX, pixelProbeY);
  ProducerScope producer(&core.rsub.producerScope, kProducerKey, "model:fixed");
  RenderNodeScope renderNode(core, draw.object);
  for (std::size_t faceIndex = 0; faceIndex < draw.faces.size(); ++faceIndex) {
    const ModelFace &face = draw.faces[faceIndex];
    const ModelFace *previousFace =
        previous && faceIndex < previous->faces.size() && canInterpolateModelFace(previous->faces[faceIndex], face)
            ? &previous->faces[faceIndex]
            : nullptr;
    std::array<psxport::native_projection::NativeProjectedVertex, 3> projected{};
    for (std::uint32_t i = 0; i < 3; ++i) {
      const ModelVertex vertex =
          previousFace ? interpolateModelVertex(previousFace->vertices[i], face.vertices[i], alpha) : face.vertices[i];
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
        {{projected[0].sx + environment.drawOffsetX, projected[0].sy + environment.drawOffsetY}},
        {{projected[1].sx + environment.drawOffsetX, projected[1].sy + environment.drawOffsetY}},
        {{projected[2].sx + environment.drawOffsetX, projected[2].sy + environment.drawOffsetY}},
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
      xs[i] = projected[i].sx + environment.drawOffsetX;
      ys[i] = projected[i].sy + environment.drawOffsetY;
      // Real frames retain retail's quantized GTE SXY coverage. A synthesized midpoint is rebuilt
      // from interpolated title-owned scene inputs, so keep the native projector's subpixel result;
      // quantizing it here would reintroduce one-pixel temporal steps without improving PSX fidelity.
      screenX[i] = previous != nullptr ? projected[i].px + static_cast<float>(environment.drawOffsetX)
                                       : static_cast<float>(xs[i]);
      screenY[i] = previous != nullptr ? projected[i].py + static_cast<float>(environment.drawOffsetY)
                                       : static_cast<float>(ys[i]);
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
    const int dither = face.textured ? (face.texturePage >> 9u) & 1u : environment.textureDither;
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
                      environment.textureWindowMaskX,
                      environment.textureWindowMaskY,
                      environment.textureWindowOffsetX,
                      environment.textureWindowOffsetY,
                      drawAreaX0,
                      environment.drawAreaY0,
                      drawAreaX1,
                      environment.drawAreaY1,
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

NativeModelSubmitResult submitFixedModel(Core &core, const ModelDraw &draw) {
  return submitFixedModel(core, draw, nullptr, 1.0f, captureRenderEnvironment(core));
}

void submitFixedModels(Core &core, SceneSnapshot &snapshot) {
  if (!snapshot.valid) {
    return;
  }
  snapshot.modelEnvironment = captureRenderEnvironment(core);
  snapshot.modelEnvironment.authoredScreenPresentation = snapshot.authoredScreenPresentation;
  for (ModelDraw &draw : snapshot.models) {
    const NativeModelSubmitResult submitted = submitFixedModel(core, draw, nullptr, 1.0f, snapshot.modelEnvironment);
    draw.nativeFacesSubmitted = submitted.submitted;
    draw.nativeZeroDepthRejected = submitted.zeroDepthRejected;
    draw.nativeFarDepthRejected = submitted.farDepthRejected;
    draw.nativeWindingRejected = submitted.windingRejected;
  }
}

} // namespace crashbash::render
