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

// Frame-wide bound on any authored order key. The key is the ordering-table word a face lands in: its
// viewport's slice plus its retail key, which retail only inserts below its slice's signed-halfword
// depthLimit. Every such word lies in the 0x1000-word table, so no key reaches 0x8000.
inline constexpr int kFixedModelSortKeyDomain = 0x8000;

// The D32 ord band carrying `sortKey`, uniform over the frame-wide key domain. It is deliberately a
// function of the KEY ALONE -- see the .cpp for why any per-draw term (depthBias, depthLimit)
// breaks frame-wide monotonicity across objects.
float fixedModelSortKeyOrd(int sortKey);

} // namespace crashbash::render
