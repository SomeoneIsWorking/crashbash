#pragma once

#include "scene_snapshot.h"

#include <cstdint>

struct Core;

namespace crashbash::render {

struct NativeModelSubmitResult {
  std::uint32_t submitted = 0;
  std::uint32_t zeroDepthRejected = 0;
  std::uint32_t farDepthRejected = 0;
  std::uint32_t windingRejected = 0;
};

NativeModelSubmitResult submitFixedModel(Core &core, const ModelDraw &draw);
void submitFixedModels(Core &core, SceneSnapshot &snapshot);

} // namespace crashbash::render
