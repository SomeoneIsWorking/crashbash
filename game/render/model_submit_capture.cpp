#include "model_submit_capture.h"

#include "core.h"
#include "crashbash_frame_driver.h"
#include "guest_execution.h"
#include "model_material_diagnostic.h"
#include "model_packet_identity_diagnostic.h"
#include "model_recipe_capture.h"
#include "model_transform_capture.h"
#include "model_transform_input_diagnostic.h"
#include "render_viewport.h"
#include "scene_snapshot.h"
#include "sprite_render_list.h"

#include <cstdint>
#include <lucent/log.h>
#include <utility>

namespace crashbash::render {
namespace {

constexpr std::uint32_t kStandardModelSubmit = 0x80019F1Cu;
constexpr std::uint32_t kAlternateModelSubmit = 0x8001DD50u;
constexpr std::uint32_t kBaseDepthBias = 0x800569C8u;
constexpr std::uint32_t kDepthLimit = 0x800569DEu;
constexpr std::uint32_t kGlobalFarColor = 0x80056868u;
constexpr std::uint32_t kGlobalDepthCueFactor = 0x800569ACu;
constexpr std::uint32_t kActiveViewportRecord = 0x800569A8u;
constexpr std::uint32_t kRenderList = 0x800569D8u;
constexpr std::uint32_t kOrderingTablePointer = 0x8005B68Cu;
constexpr std::uint32_t kDisplayDescriptor = 0x8005B698u;

// The viewport 0x80018B08 last began: the record it published and the slice it pointed the insertion
// base at. Both are what retail's own leaves read when they clip and insert this model's faces.
void captureViewport(Core &core, ModelDraw &draw) {
  const std::uint32_t record = core.mem_r32(kActiveViewportRecord);
  const std::uint32_t descriptor = core.mem_r32(kDisplayDescriptor);
  const std::uint32_t renderList = core.mem_r32(kRenderList);
  const std::uint32_t orderingTable = core.mem_r32(kOrderingTablePointer);
  if (record == 0 || descriptor == 0 || !spriteRenderListTargetsOrderingTable(renderList, orderingTable)) {
    return;
  }
  const ViewportRecord viewport{
      .left = static_cast<std::int16_t>(core.mem_r16(record)),
      .top = static_cast<std::int16_t>(core.mem_r16(record + 2u)),
      .right = static_cast<std::int16_t>(core.mem_r16(record + 4u)),
      .bottom = static_cast<std::int16_t>(core.mem_r16(record + 6u)),
  };
  draw.viewportClip = viewportClip(viewport, static_cast<std::int16_t>(core.mem_r16(descriptor + 4u)));
  draw.orderingSlice = orderingTablePosition(renderList, orderingTable, 0);
  draw.viewportValid = true;
}

ModelDraw decodeDraw(Core &core,
                     ModelSubmitter submitter,
                     std::uint32_t object,
                     std::uint32_t matrix,
                     std::uint32_t translation,
                     std::uint32_t callFlags) {
  const std::uint32_t modelAsset = core.mem_r32(object + 0x6Cu);
  std::uint16_t depthBias = core.mem_r16(kBaseDepthBias);
  const std::uint32_t objectFlags = core.mem_r32(object);
  if (submitter == ModelSubmitter::Alternate || (objectFlags & 0x02000000u) != 0) {
    depthBias = static_cast<std::uint16_t>(depthBias + core.mem_r16(object + 0x68u));
  }
  const std::array<std::int32_t, 3> objectFarColor{
      static_cast<std::int32_t>(core.mem_r32(object + 0x78u)),
      static_cast<std::int32_t>(core.mem_r32(object + 0x7Cu)),
      static_cast<std::int32_t>(core.mem_r32(object + 0x80u)),
  };
  const std::array<std::int32_t, 3> globalFarColor{
      static_cast<std::int32_t>(core.mem_r32(kGlobalFarColor)),
      static_cast<std::int32_t>(core.mem_r32(kGlobalFarColor + 4u)),
      static_cast<std::int32_t>(core.mem_r32(kGlobalFarColor + 8u)),
  };
  const ModelColorCueInputs cue =
      resolveModelColorCueInputs(((objectFlags | callFlags) & 0x40000000u) != 0,
                                 static_cast<std::int16_t>(core.mem_r16(object + 0x76u)),
                                 objectFarColor,
                                 static_cast<std::int32_t>(core.mem_r32(kGlobalDepthCueFactor)),
                                 globalFarColor);
  ModelDraw draw{
      .submitter = submitter,
      .object = object,
      .objectFlags = objectFlags,
      .matrix = matrix,
      .translation = translation,
      .callFlags = callFlags,
      .modelAsset = modelAsset,
      .modelData = modelAsset == 0 ? 0u : core.mem_r32(modelAsset + 0x0Cu),
      .frameCode = core.mem_r16(object + 0x74u),
      .depthBias = static_cast<std::int16_t>(depthBias),
      .depthLimit = static_cast<std::int16_t>(core.mem_r16(kDepthLimit)),
      .depthScale = core.rsub.projParams.zsf3(),
      .depthCueFarColor = cue.farColor,
      .depthCueFactor = cue.factor,
  };
  captureViewport(core, draw);
  return draw;
}

void recordIfRenderable(Core &core, ModelDraw draw) {
  // Retail's object submitters require render-enable plus a live model, and 0x80019A60 declines the
  // zero/sentinel frame codes before producing geometry. Recording only that accepted set makes the
  // snapshot a faithful denominator, not a list of objects the game itself declined to draw.
  if (!isRenderableModelDraw(draw)) {
    frameDriver(core).packetIdentityDiagnostic().finishDraw(draw);
    return;
  }
  SceneSnapshotHistory &history = frameDriver(core).sceneSnapshots();
  if (history.current().valid) {
    if (draw.transform.valid) {
      captureFixedModelRecipe(core, draw);
    }
    frameDriver(core).packetIdentityDiagnostic().finishDraw(draw);
    history.record(std::move(draw));
  } else {
    frameDriver(core).packetIdentityDiagnostic().finishDraw(draw);
  }
}

void standardModelSubmit(Core *core) {
  ModelDraw draw = decodeDraw(*core, ModelSubmitter::Standard, core->r[4], core->r[5], core->r[5] + 0x14u, core->r[6]);
  render::ModelTransformCapture &capture = frameDriver(*core).modelTransformCapture();
  capture.reset(*core, draw.object);
  frameDriver(*core).packetIdentityDiagnostic().beginDraw();
  runtime::callOriginal(*core, runtime::GuestImage::Resident, kStandardModelSubmit);
  if (capture.take(*core, draw.object, draw.transform)) {
    frameDriver(*core).transformInputDiagnostic().observe(draw.transform, draw.submitter);
  }
  recordIfRenderable(*core, std::move(draw));
}

void alternateModelSubmit(Core *core) {
  ModelDraw draw = decodeDraw(*core, ModelSubmitter::Alternate, core->r[4], 0u, 0u, 0u);
  render::ModelTransformCapture &capture = frameDriver(*core).modelTransformCapture();
  capture.reset(*core, draw.object);
  frameDriver(*core).packetIdentityDiagnostic().beginDraw();
  runtime::callOriginal(*core, runtime::GuestImage::Resident, kAlternateModelSubmit);
  if (capture.take(*core, draw.object, draw.transform)) {
    frameDriver(*core).transformInputDiagnostic().observe(draw.transform, draw.submitter);
  }
  recordIfRenderable(*core, std::move(draw));
}
} // namespace

void registerModelSubmitCaptureOverrides(Core &core) {
  runtime::registerNativeOverride(core,
                                  runtime::GuestImage::Resident,
                                  kStandardModelSubmit,
                                  "CrashBash::StandardModelSubmitCapture",
                                  standardModelSubmit);
  runtime::registerNativeOverride(core,
                                  runtime::GuestImage::Resident,
                                  kAlternateModelSubmit,
                                  "CrashBash::AlternateModelSubmitCapture",
                                  alternateModelSubmit);
  registerModelPacketIdentityDiagnosticOverride(core);
}

} // namespace crashbash::render
