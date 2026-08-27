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

void registerModelTransformCaptureOverride();
void resetModelTransformCapture(Core &core, std::uint32_t object);
bool takeModelTransformCapture(Core &core, std::uint32_t object, ModelTransform &out);
const ModelTransformCaptureCensus &modelTransformCaptureCensus();

} // namespace crashbash::render
