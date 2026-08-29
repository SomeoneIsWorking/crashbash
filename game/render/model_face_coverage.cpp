#include "model_face_coverage.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace crashbash::render {

float fixedModelSortKeyOrd(int sortKey, std::int16_t depthLimit) {
  // The D32 ord band that carries the game's own OT key: LINEAR in the key over the game's own
  // key domain [0, depthLimit) — the same domain retail 0x800193A8's far rejection uses — with
  // key 0 nearest (ord just under 1.0) and depthLimit-1 farthest. A 1/pz carrier was tried first
  // and is wrong twice over: pzToOrd saturates every key nearer than the near plane into one band
  // (non-injective — the Authored resolver fatal-aborts), and even normalized past the near plane
  // its 1/pz shape compresses far keys until a bucket's LIFO ties no longer fit the D32 range
  // ("bucket 789 needs 59 distinct D32 ties" at band width 4e-7). The key IS the authored order;
  // the carrier only has to be injective in the key and leave every bucket room for its ties, and
  // a uniform map maximizes the worst-case tie budget. The half-key offset centers each band so
  // adjacent bands never touch. `depthLimit` must be > 0: retail rejects every sortKey >= limit,
  // so a zero limit accepts no faces and this function is never reached.
  return 1.0f - (static_cast<float>(sortKey) + 0.5f) / static_cast<float>(depthLimit);
}

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
