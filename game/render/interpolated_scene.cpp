#include "interpolated_scene.h"

#include "core.h"
#include "crashbash_frame_driver.h"
#include "fps60_gpu_present.h"
#include "game.h"
#include "native_model_producer.h"
#include "render_queue.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>

#include <lucent/log.h>

namespace crashbash::render {
namespace {

template <typename T> T interpolateInteger(T previous, T current, float alpha) {
  if (alpha <= 0.0f) {
    return previous;
  }
  if (alpha >= 1.0f) {
    return current;
  }
  const double value =
      static_cast<double>(previous) + (static_cast<double>(current) - static_cast<double>(previous)) * alpha;
  const double bounded = std::clamp(
      value, static_cast<double>(std::numeric_limits<T>::lowest()), static_cast<double>(std::numeric_limits<T>::max()));
  return static_cast<T>(std::lround(bounded));
}

bool sameModelIdentity(const ModelDraw &left, const ModelDraw &right) {
  return left.object == right.object && left.submitter == right.submitter && left.modelAsset == right.modelAsset &&
         left.modelData == right.modelData;
}

const ModelDraw *
matchingPreviousDraw(const SceneSnapshot &previous, const SceneSnapshot &current, std::size_t currentIndex) {
  const ModelDraw &target = current.models[currentIndex];
  std::size_t occurrence = 0;
  for (std::size_t index = 0; index < currentIndex; ++index) {
    occurrence += sameModelIdentity(current.models[index], target) ? 1u : 0u;
  }
  for (const ModelDraw &candidate : previous.models) {
    if (!sameModelIdentity(candidate, target)) {
      continue;
    }
    if (occurrence == 0) {
      return &candidate;
    }
    --occurrence;
  }
  return nullptr;
}

bool isNativeModelItem(const RqItem &item) {
  return item.layer == RQ_WORLD && item.has_xyf != 0 && item.guest_packet == 0 && item.dbg_node != 0;
}

class QueueRedirect final {
public:
  QueueRedirect(Game &game, RenderQueue &queue) : game_(game), previous_(game.rqRedirect) {
    game_.rqRedirect = &queue;
  }
  ~QueueRedirect() {
    game_.rqRedirect = previous_;
  }
  QueueRedirect(const QueueRedirect &) = delete;
  QueueRedirect &operator=(const QueueRedirect &) = delete;

private:
  Game &game_;
  RenderQueue *previous_;
};

} // namespace

ModelTransform interpolateModelTransform(const ModelTransform &previous, const ModelTransform &current, float alpha) {
  if (!previous.valid || alpha >= 1.0f) {
    return current;
  }
  if (!current.valid || alpha <= 0.0f) {
    return previous;
  }
  ModelTransform result = current;
  for (std::size_t row = 0; row < result.rotation.size(); ++row) {
    for (std::size_t column = 0; column < result.rotation[row].size(); ++column) {
      result.rotation[row][column] =
          interpolateInteger(previous.rotation[row][column], current.rotation[row][column], alpha);
    }
    result.translation[row] = interpolateInteger(previous.translation[row], current.translation[row], alpha);
  }
  result.projectionX = interpolateInteger(previous.projectionX, current.projectionX, alpha);
  result.projectionY = interpolateInteger(previous.projectionY, current.projectionY, alpha);
  result.projectionDistance = interpolateInteger(previous.projectionDistance, current.projectionDistance, alpha);
  result.valid = true;
  return result;
}

ModelVertex interpolateModelVertex(const ModelVertex &previous, const ModelVertex &current, float alpha) {
  if (alpha <= 0.0f) {
    return previous;
  }
  if (alpha >= 1.0f) {
    return current;
  }
  return {
      .x = interpolateInteger(previous.x, current.x, alpha),
      .y = interpolateInteger(previous.y, current.y, alpha),
      .z = interpolateInteger(previous.z, current.z, alpha),
      .flags = current.flags,
  };
}

bool canInterpolateModelFace(const ModelFace &previous, const ModelFace &current) {
  return previous.sourceFace == current.sourceFace && previous.sourceGroup == current.sourceGroup &&
         previous.sourceGroupFace == current.sourceGroupFace && previous.sourceMaterial == current.sourceMaterial &&
         previous.topologyFlags == current.topologyFlags && previous.textured == current.textured;
}

InterpolatedScenePresentation::InterpolatedScenePresentation(Game &game) : game_(game) {}

InterpolatedScenePresentation::~InterpolatedScenePresentation() = default;

bool InterpolatedScenePresentation::active(const Core &core) const {
  return game_.mods.fps60 != 0 && core.rsub.mode.enhancementsAllowed();
}

long InterpolatedScenePresentation::buildMidpoint(Core &core,
                                                  const SceneSnapshot &previous,
                                                  const SceneSnapshot &current) {
  if (!midpointQueue_) {
    midpointQueue_ = std::make_unique<RenderQueue>();
    midpointQueue_->game = &game_;
  }
  midpointQueue_->reset();
  {
    QueueRedirect redirect(game_, *midpointQueue_);
    for (std::size_t index = 0; index < current.models.size(); ++index) {
      submitFixedModel(
          core, current.models[index], matchingPreviousDraw(previous, current, index), 0.5f, current.modelEnvironment);
    }
  }
  midpointQueue_->finalize(&core, "crashbash-interpolated-scene");
  return midpointQueue_->n;
}

void InterpolatedScenePresentation::buildPresentationStream(CapturedFrameView frame, bool replaceModels) {
  presentationStream_.clear();
  const std::size_t midpointCount = replaceModels ? static_cast<std::size_t>(midpointQueue_->n) : 0u;
  presentationStream_.reserve(frame.items.size() + midpointCount);

  const std::size_t capturedModelCount =
      replaceModels ? static_cast<std::size_t>(std::count_if(frame.items.begin(), frame.items.end(), isNativeModelItem))
                    : 0u;
  if (replaceModels && capturedModelCount == 0) {
    lucent::error("fps60", "Crash Bash built an interpolated model scene but captured no replaceable model items");
    std::abort();
  }

  // Model faces can occupy several authored-order runs with verbatim world primitives between them.
  // Preserve those insertion boundaries by distributing the sorted midpoint stream across the exact
  // captured model positions. Equal face counts replace one-for-one; visibility changes distribute the
  // smaller/larger midpoint set monotonically without moving the intervening 2D primitives.
  std::size_t capturedModelsSeen = 0;
  std::size_t midpointEmitted = 0;
  for (const RqItem &item : frame.items) {
    const bool model = isNativeModelItem(item);
    if (replaceModels && model) {
      ++capturedModelsSeen;
      const std::size_t midpointTarget = capturedModelsSeen * midpointCount / capturedModelCount;
      while (midpointEmitted < midpointTarget) {
        presentationStream_.push_back(&midpointQueue_->items[midpointEmitted++]);
      }
      continue;
    }
    presentationStream_.push_back(&item);
  }
  if (midpointEmitted != midpointCount) {
    lucent::error("fps60",
                  "Crash Bash midpoint merge emitted {} of {} interpolated model primitives",
                  midpointEmitted,
                  midpointCount);
    std::abort();
  }
}

void InterpolatedScenePresentation::present(FramePresentationBackend &backend,
                                            Core &core,
                                            CapturedFrameView frame,
                                            int guestFields) {
  SceneSnapshotHistory &history = frameDriver(core).sceneSnapshots();
  const bool extraFrame = active(core) && guestFields == 2 && history.temporalPairValid();
  long interpolatedPrims = 0;
  if (extraFrame) {
    bool hasModelItems = false;
    for (const RqItem &item : frame.items) {
      hasModelItems = hasModelItems || isNativeModelItem(item);
    }
    if (hasModelItems) {
      interpolatedPrims = buildMidpoint(core, history.previous(), history.presentable());
    }
    buildPresentationStream(frame, hasModelItems);
    game_.rq.emitItemStream(&core, presentationStream_);
    gpu_fps60_present_pass(&core);
    backend.captureDiagnostic(frame.fence, true);
    lucent::debug("fps60",
                  "f{} slotA: source snapshots {}->{} n={} tier1={} backdrop=0 t=0.500",
                  frame.fence,
                  history.previous().logicFrame,
                  history.presentable().logicFrame,
                  frame.items.size(),
                  interpolatedPrims);
    backend.pace(guestFields, 2);
  }

  backend.emit(frame.items);
  backend.presentReal();
  backend.captureDiagnostic(frame.fence, false);
  backend.pace(guestFields, extraFrame ? 2 : 1);
  history.markPresented();
}

} // namespace crashbash::render
