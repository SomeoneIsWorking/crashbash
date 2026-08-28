#include "model_face_coverage.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace crashbash::render {

std::uint16_t fixedModelAvsz3Otz(const std::array<ProjectedFaceVertex, 3> &vertices, std::int16_t depthScale) {
  const std::uint64_t depthSum = static_cast<std::uint64_t>(vertices[0].depth) + vertices[1].depth + vertices[2].depth;
  const std::int64_t average = static_cast<std::int64_t>(depthScale) * static_cast<std::int64_t>(depthSum);
  if (average > std::numeric_limits<std::int32_t>::max()) {
    return std::numeric_limits<std::uint16_t>::max();
  }
  if (average < std::numeric_limits<std::int32_t>::min()) {
    return 0;
  }
  const auto mac0 = static_cast<std::int32_t>(average);
  const std::int32_t shifted =
      mac0 >= 0 ? mac0 >> 12u : -static_cast<std::int32_t>((-static_cast<std::int64_t>(mac0) + 0xFFF) >> 12u);
  return static_cast<std::uint16_t>(
      std::clamp(shifted, 0, static_cast<std::int32_t>(std::numeric_limits<std::uint16_t>::max())));
}

ModelFaceCoverage classifyFixedModelFace(const std::array<ProjectedFaceVertex, 3> &vertices,
                                         bool textured,
                                         std::uint16_t faceFlags,
                                         std::int16_t depthBias,
                                         std::int16_t depthLimit,
                                         std::int16_t depthScale) {
  const std::uint32_t otz = fixedModelAvsz3Otz(vertices, depthScale);
  if (!textured && otz == 0) {
    return {.rejection = ModelFaceRejection::ZeroUntexturedDepth};
  }

  const std::uint32_t biasedDepth = otz + static_cast<std::uint32_t>(depthBias);
  const std::uint32_t sortKey = biasedDepth >> 1u;
  const std::uint32_t limit = static_cast<std::uint32_t>(depthLimit);
  if (sortKey >= limit) {
    return {.rejection = ModelFaceRejection::FarDepth, .sortKey = sortKey};
  }
  if ((faceFlags & 2u) != 0) {
    return {.sortKey = sortKey};
  }

  const std::int64_t area = static_cast<std::int64_t>(vertices[0].x) * vertices[1].y +
                            static_cast<std::int64_t>(vertices[1].x) * vertices[2].y +
                            static_cast<std::int64_t>(vertices[2].x) * vertices[0].y -
                            static_cast<std::int64_t>(vertices[0].x) * vertices[2].y -
                            static_cast<std::int64_t>(vertices[1].x) * vertices[0].y -
                            static_cast<std::int64_t>(vertices[2].x) * vertices[1].y;
  const bool frontFacing = (faceFlags & 1u) == 0 ? area > 0 : area <= 0;
  return {
      .rejection = frontFacing ? ModelFaceRejection::None : ModelFaceRejection::Winding,
      .sortKey = sortKey,
  };
}

} // namespace crashbash::render
