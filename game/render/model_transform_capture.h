#pragma once

#include "scene_snapshot.h"

struct Core;

namespace crashbash::render {

ModelRotation composeModelRotations(const ModelRotation &left, const ModelRotation &right);
std::array<std::int32_t, 3> transformLargeModelTranslation(const ModelRotation &rotation,
                                                           const std::array<std::int32_t, 3> &translation);

struct ModelTransformCaptureCensus {
  std::uint32_t attempts = 0;
  std::uint32_t pendingMismatch = 0;
  std::uint32_t invalidOutput = 0;
  std::uint32_t invalidRotation = 0;
  std::uint32_t missingTranslation = 0;
  std::uint32_t invalidCamera = 0;
  std::uint32_t invalidProjection = 0;
  std::uint32_t captured = 0;
  std::uint32_t lastOutput = 0;
  std::uint32_t lastRotation = 0;
  std::uint32_t lastTranslation = 0;
  std::uint32_t lastCamera = 0;
  std::uint32_t lastProjection = 0;
};

// The one-shot handoff between the retail transform composer and the object submit leaf: `reset`
// names the object whose transform is about to be composed, the composer fills it, and `take`
// consumes it once. Product state, not evidence — one instance belongs to the frame driver, and
// `census` is what the composer rejected on the way.
class ModelTransformCapture {
public:
  void reset(Core &core, std::uint32_t object);
  bool take(Core &core, std::uint32_t object, ModelTransform &out);
  void compose(Core &core);
  void composeAlternate(Core &core);

  const ModelTransformCaptureCensus &census() const {
    return census_;
  }

private:
  struct Pending {
    Core *core = nullptr;
    std::uint32_t object = 0;
    ModelTransform transform;
  };

  Pending pending_{};
  ModelTransformCaptureCensus census_{};
};

void registerModelTransformCaptureOverride(Core &core);

} // namespace crashbash::render
