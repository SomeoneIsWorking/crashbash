#include "model_face_coverage.h"

#include "gte_state.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <limits>

namespace {

using crashbash::render::classifyFixedModelFace;
using crashbash::render::fixedModelAvsz3Otz;
using crashbash::render::ModelFaceRejection;
using crashbash::render::ProjectedFaceVertex;

constexpr std::array<ProjectedFaceVertex, 3> kFrontFacing{{
    {.x = 0, .y = 0, .depth = 20},
    {.x = 2, .y = 0, .depth = 20},
    {.x = 0, .y = 2, .depth = 20},
}};

constexpr std::int16_t kRetailZsf3 = 341;

bool avszMatchesGuest(const std::array<ProjectedFaceVertex, 3> &vertices, std::int16_t zsf3) {
  GteRegs guest{};
  guest.REG[17] = vertices[0].depth;
  guest.REG[18] = vertices[1].depth;
  guest.REG[19] = vertices[2].depth;
  guest.REG[32 + 29] = static_cast<std::uint16_t>(zsf3);
  return GTE_ExecuteIsolated(&guest, 0x4B58002Du) >= 0 &&
         fixedModelAvsz3Otz(vertices, zsf3) == static_cast<std::uint16_t>(guest.REG[7]);
}

bool check(bool condition) {
  return condition;
}

} // namespace

int test_sort_key_ord_monotone(void);

int main() {
  if (test_sort_key_ord_monotone() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (!check(avszMatchesGuest(kFrontFacing, kRetailZsf3))) {
    return EXIT_FAILURE;
  }
  constexpr std::array<ProjectedFaceVertex, 3> measuredFace{{
      {.x = 208, .y = 176, .depth = 6134},
      {.x = 270, .y = 220, .depth = 5600},
      {.x = 241, .y = 159, .depth = 6418},
  }};
  if (!check(avszMatchesGuest(measuredFace, kRetailZsf3) && fixedModelAvsz3Otz(measuredFace, kRetailZsf3) == 1511u)) {
    return EXIT_FAILURE;
  }
  constexpr std::array<ProjectedFaceVertex, 3> maximumDepth{{
      {.depth = 0xFFFFu},
      {.depth = 0xFFFFu},
      {.depth = 0xFFFFu},
  }};
  if (!check(avszMatchesGuest(maximumDepth, std::numeric_limits<std::int16_t>::max()) &&
             avszMatchesGuest(maximumDepth, std::numeric_limits<std::int16_t>::min()))) {
    return EXIT_FAILURE;
  }

  const auto measuredAccepted = classifyFixedModelFace(measuredFace, false, 2u, 2000, 4096, kRetailZsf3);
  if (!check(measuredAccepted.accepted() && measuredAccepted.sortKey == 1755u)) {
    return EXIT_FAILURE;
  }

  const auto accepted = classifyFixedModelFace(kFrontFacing, false, 0u, 4, 100, kRetailZsf3);
  if (!check(accepted.accepted() && accepted.sortKey == 4u)) {
    return EXIT_FAILURE;
  }

  auto zeroThird = kFrontFacing;
  zeroThird[0].depth = 1;
  zeroThird[1].depth = 1;
  zeroThird[2].depth = 1;
  if (!check(classifyFixedModelFace(zeroThird, false, 2u, 4, 100, kRetailZsf3).rejection ==
             ModelFaceRejection::ZeroUntexturedDepth)) {
    return EXIT_FAILURE;
  }
  if (!check(classifyFixedModelFace(zeroThird, true, 2u, 4, 100, kRetailZsf3).accepted())) {
    return EXIT_FAILURE;
  }

  if (!check(classifyFixedModelFace(maximumDepth, false, 2u, 0, 10, kRetailZsf3).rejection ==
             ModelFaceRejection::FarDepth)) {
    return EXIT_FAILURE;
  }
  if (!check(classifyFixedModelFace(kFrontFacing, false, 0u, 0, 100, kRetailZsf3).accepted())) {
    return EXIT_FAILURE;
  }
  if (!check(classifyFixedModelFace(kFrontFacing, false, 1u, 0, 100, kRetailZsf3).rejection ==
             ModelFaceRejection::Winding)) {
    return EXIT_FAILURE;
  }
  if (!check(classifyFixedModelFace(kFrontFacing, false, 2u, 0, 100, kRetailZsf3).accepted())) {
    return EXIT_FAILURE;
  }

  const auto wrapped = classifyFixedModelFace(kFrontFacing, false, 2u, -5, 100, kRetailZsf3);
  if (!check(wrapped.rejection == ModelFaceRejection::FarDepth && wrapped.sortKey == 0x7FFFFFFFu)) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

// The key's ord carrier must be strictly decreasing in the key over the game's whole key domain
// [0, depthLimit), stay in (0,1), and leave every bucket enough D32 room for its LIFO ties. The
// pzToOrd(key*2) mapping collapsed all near keys into one band and fatal-aborted the render
// (watchdog FATAL at frame 256+ of the attract flow); the 1/pz-shaped retry then starved a far
// bucket of tie room ("bucket 789 needs 59 distinct D32 ties").
int test_sort_key_ord_monotone(void) {
  using crashbash::render::fixedModelSortKeyOrd;
  constexpr std::int16_t kLimit = 2048;
  const float ord0 = fixedModelSortKeyOrd(0, kLimit);
  const float ordLast = fixedModelSortKeyOrd(kLimit - 1, kLimit);
  if (!check(ord0 < 1.0f && ord0 > 1.0f - 2.0f / kLimit)) {
    return EXIT_FAILURE;
  }
  if (!check(ordLast > 0.0f && ordLast < 1.0f / kLimit)) {
    return EXIT_FAILURE;
  }
  float prev = ord0;
  for (int key = 1; key < kLimit; ++key) {
    const float ord = fixedModelSortKeyOrd(key, kLimit);
    if (!check(ord < prev && ord > 0.0f)) {
      return EXIT_FAILURE;
    }
    prev = ord;
  }
  // Adjacent-key band width is uniform (1/limit), so the worst-case bucket at the far end still
  // has thousands of D32-distinct tie slots, not the 6 that broke the attract flow.
  const float band = fixedModelSortKeyOrd(0, kLimit) - fixedModelSortKeyOrd(1, kLimit);
  const float bandFar = fixedModelSortKeyOrd(kLimit - 2, kLimit) - fixedModelSortKeyOrd(kLimit - 1, kLimit);
  if (!check(band == bandFar && band > 4000.0f * (1.0f / 16777216.0f))) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
