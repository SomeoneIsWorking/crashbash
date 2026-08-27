#pragma once

#include <array>
#include <cstdint>

namespace crashbash::render {

struct ProjectedFaceVertex {
  std::int16_t x = 0;
  std::int16_t y = 0;
  std::uint16_t depth = 0;
};

enum class ModelFaceRejection : std::uint8_t {
  None,
  ZeroUntexturedDepth,
  FarDepth,
  Winding,
};

struct ModelFaceCoverage {
  ModelFaceRejection rejection = ModelFaceRejection::None;
  std::uint32_t sortKey = 0;

  bool accepted() const {
    return rejection == ModelFaceRejection::None;
  }
};

// Source-owned coverage at retail 0x800193A8. `depthBias` and `depthLimit` are the title's signed
// halfwords; unsigned MIPS addition/shift/comparison semantics are preserved intentionally.
ModelFaceCoverage classifyFixedModelFace(const std::array<ProjectedFaceVertex, 3> &vertices,
                                         bool textured,
                                         std::uint16_t faceFlags,
                                         std::int16_t depthBias,
                                         std::int16_t depthLimit);

} // namespace crashbash::render
