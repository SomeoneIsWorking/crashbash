#include "model_face_coverage.h"

#include <cstdint>

namespace crashbash::render {

ModelFaceCoverage classifyFixedModelFace(const std::array<ProjectedFaceVertex, 3> &vertices,
                                         bool textured,
                                         std::uint16_t faceFlags,
                                         std::int16_t depthBias,
                                         std::int16_t depthLimit) {
  if (!textured && vertices[2].depth == 0) {
    return {.rejection = ModelFaceRejection::ZeroUntexturedDepth};
  }

  const std::uint32_t biasedDepth =
      static_cast<std::uint32_t>(vertices[2].depth) + static_cast<std::uint32_t>(depthBias);
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
