#include "model_submit_capture.h"

#include "core.h"
#include "crashbash_frame_driver.h"
#include "guest_packet_filter.h"
#include "model_material_diagnostic.h"
#include "model_packet_identity_diagnostic.h"
#include "model_recipe_capture.h"
#include "model_transform_capture.h"
#include "model_transform_input_diagnostic.h"
#include "override_registry.h"
#include "scene_snapshot.h"

#include <cstdint>
#include <cstdlib>
#include <lucent/log.h>
#include <utility>

#ifdef CRASHBASH_HAVE_SUBSTRATE
#include "rec_decls.h"
#endif

namespace crashbash::render {
namespace {

#ifdef CRASHBASH_HAVE_SUBSTRATE
constexpr std::uint32_t kStandardModelSubmit = 0x80019F1Cu;
constexpr std::uint32_t kAlternateModelSubmit = 0x8001DD50u;
constexpr std::uint32_t kBaseDepthBias = 0x800569C8u;
constexpr std::uint32_t kDepthLimit = 0x800569DEu;
constexpr std::uint32_t kGlobalFarColor = 0x80056868u;
constexpr std::uint32_t kGlobalDepthCueFactor = 0x800569ACu;

struct PacketOwnershipRuntime {
  ModelPacketOwnership ownership;
  std::uint32_t logicFrame = 0;
  bool frameKnown = false;
};

thread_local PacketOwnershipRuntime packetOwnership;

std::size_t submitterIndex(ModelSubmitter submitter) {
  return submitter == ModelSubmitter::Standard ? 0u : 1u;
}

std::uint32_t submitterAddress(ModelSubmitter submitter) {
  return submitter == ModelSubmitter::Standard ? kStandardModelSubmit : kAlternateModelSubmit;
}

std::uint32_t beginPacketOwnershipFrame(Core &core) {
  const SceneSnapshotHistory &history = frameDriver(core).sceneSnapshots();
  if (!history.current().valid) {
    std::abort();
  }
  const std::uint32_t logicFrame = history.current().logicFrame;
  if (!packetOwnership.frameKnown || packetOwnership.logicFrame != logicFrame) {
    packetOwnership.logicFrame = logicFrame;
    packetOwnership.frameKnown = true;
    packetOwnership.ownership.beginLogicFrame(logicFrame);
  }
  return logicFrame;
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
  return ModelDraw{
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
}

bool recordIfRenderable(Core &core, ModelDraw draw) {
  // Retail's object submitters require render-enable plus a live model, and 0x80019A60 declines the
  // zero/sentinel frame codes before producing geometry. Recording only that accepted set makes the
  // snapshot a faithful denominator, not a list of objects the game itself declined to draw.
  if (!isRenderableModelDraw(draw)) {
    finishModelPacketIdentityDraw(draw);
    return false;
  }
  SceneSnapshotHistory &history = frameDriver(core).sceneSnapshots();
  if (history.current().valid) {
    if (draw.transform.valid) {
      captureFixedModelRecipe(core, draw);
    }
    const bool complete = draw.transform.valid && !draw.faces.empty();
    finishModelPacketIdentityDraw(draw);
    history.record(std::move(draw));
    return complete;
  } else {
    finishModelPacketIdentityDraw(draw);
    return false;
  }
}

void standardModelSubmit(Core *core) {
  beginPacketOwnershipFrame(*core);
  ModelDraw draw = decodeDraw(*core, ModelSubmitter::Standard, core->r[4], core->r[5], core->r[5] + 0x14u, core->r[6]);
  resetModelTransformCapture(*core, draw.object);
  beginModelPacketIdentityDraw();
  {
    GuestPacketOwnerScope packetOwner(&core->rsub.guestPacketFilter, kStandardModelSubmit);
    gen_func_80019F1C(core);
  }
  if (takeModelTransformCapture(*core, draw.object, draw.transform)) {
    observeInstalledModelTransformInputs(draw.transform, draw.submitter);
  }
  const bool complete = recordIfRenderable(*core, std::move(draw));
  packetOwnership.ownership.noteNativeSnapshot(ModelSubmitter::Standard, complete);
}

void alternateModelSubmit(Core *core) {
  beginPacketOwnershipFrame(*core);
  ModelDraw draw = decodeDraw(*core, ModelSubmitter::Alternate, core->r[4], 0u, 0u, 0u);
  resetModelTransformCapture(*core, draw.object);
  beginModelPacketIdentityDraw();
  {
    GuestPacketOwnerScope packetOwner(&core->rsub.guestPacketFilter, kAlternateModelSubmit);
    gen_func_8001DD50(core);
  }
  if (takeModelTransformCapture(*core, draw.object, draw.transform)) {
    observeInstalledModelTransformInputs(draw.transform, draw.submitter);
  }
  const bool complete = recordIfRenderable(*core, std::move(draw));
  packetOwnership.ownership.noteNativeSnapshot(ModelSubmitter::Alternate, complete);
}
#endif

} // namespace

ModelPacketSuppressionScope::ModelPacketSuppressionScope(Core &core, const SceneSnapshot &renderedSnapshot)
    : core_(&core) {
#ifdef CRASHBASH_HAVE_SUBSTRATE
  const std::uint32_t logicFrame = beginPacketOwnershipFrame(core);
  constexpr std::array<ModelSubmitter, 2> submitters = {ModelSubmitter::Standard, ModelSubmitter::Alternate};
  for (const ModelSubmitter submitter : submitters) {
    if (!packetOwnership.ownership.completeFor(logicFrame, submitter) ||
        !nativeSnapshotCompleteForSubmitter(renderedSnapshot, submitter)) {
      continue;
    }
    core.rsub.guestPacketFilter.setSuppressed(submitterAddress(submitter), true);
    active_[submitterIndex(submitter)] = true;
  }
#else
  (void)renderedSnapshot;
#endif
}

ModelPacketSuppressionScope::~ModelPacketSuppressionScope() {
#ifdef CRASHBASH_HAVE_SUBSTRATE
  if (core_ == nullptr) {
    return;
  }
  constexpr std::array<ModelSubmitter, 2> submitters = {ModelSubmitter::Standard, ModelSubmitter::Alternate};
  for (const ModelSubmitter submitter : submitters) {
    if (active_[submitterIndex(submitter)]) {
      core_->rsub.guestPacketFilter.setSuppressed(submitterAddress(submitter), false);
    }
  }
#endif
}

void registerModelSubmitCaptureOverrides() {
#ifdef CRASHBASH_HAVE_SUBSTRATE
  overrides::install(kStandardModelSubmit,
                     "CrashBash::StandardModelSubmitCapture",
                     standardModelSubmit,
                     gen_func_80019F1C,
                     shard_set_override);
  overrides::install(kAlternateModelSubmit,
                     "CrashBash::AlternateModelSubmitCapture",
                     alternateModelSubmit,
                     gen_func_8001DD50,
                     shard_set_override);
  registerModelPacketIdentityDiagnosticOverride();
#else
  lucent::debug("crashbash-render", "model submit capture overrides deferred: no generated substrate");
#endif
}

} // namespace crashbash::render
