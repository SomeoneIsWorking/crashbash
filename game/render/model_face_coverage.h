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

// Exact AVSZ3 endpoint used by retail 0x800193A8: the GTE multiplies unsigned SZ1+SZ2+SZ3 by signed
// CR29/ZSF3, writes signed MAC0, shifts by 12, then clamps OTZ to an unsigned halfword.
std::uint16_t fixedModelAvsz3Otz(const std::array<ProjectedFaceVertex, 3> &vertices, std::int16_t depthScale);

// Source-owned coverage at retail 0x800193A8. `depthBias`, `depthLimit`, and `depthScale` are the
// title's signed halfwords; unsigned MIPS addition/shift/comparison semantics are preserved.
ModelFaceCoverage classifyFixedModelFace(const std::array<ProjectedFaceVertex, 3> &vertices,
                                         bool textured,
                                         std::uint16_t faceFlags,
                                         std::int16_t depthBias,
                                         std::int16_t depthLimit,
                                         std::int16_t depthScale);

// The D32 ord band carrying `sortKey`, linear over the game's own key domain [0, depthLimit) —
// see the .cpp for why the carrier is uniform in the key rather than affine in 1/pz.
float fixedModelSortKeyOrd(int sortKey, std::int16_t depthLimit);

} // namespace crashbash::render
