#pragma once

#include "scene_snapshot.h"

#include <cstdint>

struct Core;

namespace crashbash::render {

enum class ModelRecipeStatus {
  UnsupportedFrame,
  InvalidSource,
  ValidEmpty,
  Ready,
};

struct ModelRecipeCensus {
  ModelRecipeStatus status = ModelRecipeStatus::UnsupportedFrame;
  std::uint32_t groups = 0;
  std::uint32_t faces = 0;
  std::uint32_t texturedFaces = 0;
};

ModelRecipeCensus captureFixedModelRecipe(Core &core, ModelDraw &draw);

} // namespace crashbash::render
