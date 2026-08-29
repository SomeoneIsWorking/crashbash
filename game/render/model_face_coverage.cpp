#include "model_face_coverage.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace crashbash::render {

float fixedModelSortKeyOrd(int sortKey) {
  // The D32 ord band that carries the game's own OT key. The OT is ONE frame-wide table and retail
  // orders it strictly by key, so this carrier must be a function of the KEY ALONE: any per-draw
  // term makes the same key land in different bands for different objects, and the frame-wide
  // monotonicity `rq_apply_ot_lifo_depths` requires is then violated across an object boundary.
  //
  // This was measured, not reasoned: normalizing by the per-draw `depthLimit` fatal-aborted the
  // attract flow at key 187 with ord 0.947654963 against a nearer band of 0.728962779 — the same
  // key under limits 3582 and ~692. It stayed latent until the DAT28272 module put draws with two
  // different limits in one frame. Per-object `depthBias` was excluded from the carrier for exactly
  // this reason (it is already baked into the key); `depthLimit` is the same mistake one step on.
  //
  // The denominator is therefore the key domain's own frame-wide bound, taken from the game's
  // types rather than tuned: retail rejects every face whose key >= depthLimit, and depthLimit is a
  // signed halfword, so every ACCEPTED key is < 0x8000 whatever a given draw's limit happens to be.
  // A uniform band of 1/0x8000 (~3.05e-5 of the ord range) leaves each bucket far more D32 tie room
  // than the 4e-7 that starved bucket 789 under the earlier 1/pz-shaped carrier. The half-key
  // offset centers each band so adjacent bands never touch.
  //
  // A 1/pz carrier is wrong twice over and must not come back: pzToOrd saturates every key nearer
  // than the near plane into one band (non-injective -- the Authored resolver fatal-aborts), and
  // even normalized past the near plane its shape compresses far keys until a bucket's LIFO ties no
  // longer fit. The key IS the authored order; the carrier only has to be injective in the key,
  // frame-wide monotone, and leave every bucket room for its ties. A uniform map maximizes the
  // worst-case tie budget.
  return 1.0f - (static_cast<float>(sortKey) + 0.5f) / static_cast<float>(kFixedModelSortKeyDomain);
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
