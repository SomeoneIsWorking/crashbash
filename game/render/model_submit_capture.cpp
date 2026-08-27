#include "model_submit_capture.h"

#include "core.h"
#include "crashbash_frame_driver.h"
#include "model_recipe_capture.h"
#include "model_transform_capture.h"
#include "native_model_producer.h"
#include "override_registry.h"
#include "scene_snapshot.h"

#include <cstdint>
#include <lucent/log.h>
#include <utility>

#ifdef CRASHBASH_HAVE_SUBSTRATE
#include "rec_decls.h"
#endif

namespace crashbash::render {
namespace {

constexpr std::uint32_t kStandardModelSubmit = 0x80019F1Cu;
constexpr std::uint32_t kAlternateModelSubmit = 0x8001DD50u;

#ifdef CRASHBASH_HAVE_SUBSTRATE
ModelDraw decodeDraw(Core &core,
                     ModelSubmitter submitter,
                     std::uint32_t object,
                     std::uint32_t matrix,
                     std::uint32_t translation,
                     std::uint32_t callFlags) {
  const std::uint32_t modelAsset = core.mem_r32(object + 0x6Cu);
  return ModelDraw{
      .submitter = submitter,
      .object = object,
      .objectFlags = core.mem_r32(object),
      .matrix = matrix,
      .translation = translation,
      .callFlags = callFlags,
      .modelAsset = modelAsset,
      .modelData = modelAsset == 0 ? 0u : core.mem_r32(modelAsset + 0x0Cu),
      .frameCode = core.mem_r16(object + 0x74u),
  };
}

void recordIfRenderable(Core &core, ModelDraw draw) {
  // Retail's object submitters require render-enable plus a live model, and 0x80019A60 declines the
  // zero/sentinel frame codes before producing geometry. Recording only that accepted set makes the
  // snapshot a faithful denominator, not a list of objects the game itself declined to draw.
  if (!isRenderableModelDraw(draw)) {
    return;
  }
  SceneSnapshotHistory &history = frameDriver(core).sceneSnapshots();
  if (history.current().valid) {
    if (draw.transform.valid) {
      captureFixedModelRecipe(core, draw);
    }
    ModelDraw &stored = history.record(std::move(draw));
    stored.nativeFacesSubmitted = submitFixedModel(core, stored);
  }
}

void standardModelSubmit(Core *core) {
  ModelDraw draw = decodeDraw(*core, ModelSubmitter::Standard, core->r[4], core->r[5], core->r[5] + 0x14u, core->r[6]);
  resetModelTransformCapture(*core, draw.object);
  gen_func_80019F1C(core);
  takeModelTransformCapture(*core, draw.object, draw.transform);
  recordIfRenderable(*core, std::move(draw));
}

void alternateModelSubmit(Core *core) {
  ModelDraw draw = decodeDraw(*core, ModelSubmitter::Alternate, core->r[4], 0u, 0u, 0u);
  gen_func_8001DD50(core);
  recordIfRenderable(*core, std::move(draw));
}
#endif

} // namespace

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
#else
  lucent::debug("crashbash-render", "model submit capture overrides deferred: no generated substrate");
#endif
}

} // namespace crashbash::render
