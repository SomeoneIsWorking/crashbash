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
int test_sort_key_ord_is_independent_of_any_per_draw_term(void);

int main() {
  if (test_sort_key_ord_monotone() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (test_sort_key_ord_is_independent_of_any_per_draw_term() != EXIT_SUCCESS) {
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
  using crashbash::render::kFixedModelSortKeyDomain;
  constexpr int kDomain = kFixedModelSortKeyDomain;
  const float ord0 = fixedModelSortKeyOrd(0);
  const float ordLast = fixedModelSortKeyOrd(kDomain - 1);
  if (!check(ord0 < 1.0f && ord0 > 1.0f - 2.0f / kDomain)) {
    return EXIT_FAILURE;
  }
  if (!check(ordLast > 0.0f && ordLast < 1.0f / kDomain)) {
    return EXIT_FAILURE;
  }
  float prev = ord0;
  for (int key = 1; key < kDomain; ++key) {
    const float ord = fixedModelSortKeyOrd(key);
    if (!check(ord < prev && ord > 0.0f)) {
      return EXIT_FAILURE;
    }
    prev = ord;
  }
  // Adjacent-key band width is uniform (1/domain), so every bucket -- near or far -- gets the same
  // tie budget, rather than the 6 slots the old 1/pz carrier left the far end.
  //
  // Budget in real terms: a band of width 1/domain maps into the renderer's 3D depth band (7/8 of
  // the D32 range) and a float32 ulp up there is 2^-24, so each bucket holds about
  // (7/8) / 32768 / 2^-24 ~= 448 distinct depths. The bar is the worst requirement ever MEASURED --
  // bucket 789 needed 59 ties under the old carrier -- not a number tuned to whatever domain is in
  // force today, which is what the previous `> 4000 ulps` assertion had silently become when the
  // domain was 2048.
  constexpr float kD32Ulp = 1.0f / 16777216.0f; // float32 ulp in the upper half of the depth band
  constexpr float k3dBandFraction = 0.875f;     // kGpuNative3dMax - kGpuNative3dMin
  constexpr float kWorstMeasuredTies = 59.0f;
  const float band = fixedModelSortKeyOrd(0) - fixedModelSortKeyOrd(1);
  const float bandFar = fixedModelSortKeyOrd(kDomain - 2) - fixedModelSortKeyOrd(kDomain - 1);
  const float slots = band * k3dBandFraction / kD32Ulp;
  if (!check(band == bandFar && slots > 4.0f * kWorstMeasuredTies)) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

// The carrier must depend on the KEY ALONE. Normalizing by the per-draw depthLimit fatal-aborted
// the attract flow once the DAT28272 module put two limits in one frame: key 187 carried ord
// 0.947654963 under limit 3582 while a nearer band sat at 0.728962779 under ~692, so the frame-wide
// order `rq_apply_ot_lifo_depths` enforces was inverted across an object boundary. Replay that exact
// pair, plus the general property, against the real function.
int test_sort_key_ord_is_independent_of_any_per_draw_term(void) {
  using crashbash::render::fixedModelSortKeyOrd;
  // The measured pair: both objects' key 187 must now carry ONE ord, and a nearer key must sit
  // strictly farther from 0 than a farther key regardless of which object submitted it.
  if (!check(fixedModelSortKeyOrd(187) == fixedModelSortKeyOrd(187))) {
    return EXIT_FAILURE;
  }
  const float nearer = fixedModelSortKeyOrd(186);
  const float here = fixedModelSortKeyOrd(187);
  const float farther = fixedModelSortKeyOrd(188);
  if (!check(nearer > here && here > farther)) {
    return EXIT_FAILURE;
  }
  // Interleave keys the way two objects with different limits would have: every key still maps to
  // one band, and the frame-wide sequence is strictly decreasing.
  const int keys[] = {0, 1, 186, 187, 188, 691, 692, 3581, 3582, 20000, 32767};
  float previous = 1.0f;
  for (const int key : keys) {
    const float ord = fixedModelSortKeyOrd(key);
    if (!check(ord < previous && ord > 0.0f)) {
      return EXIT_FAILURE;
    }
    previous = ord;
  }
  return EXIT_SUCCESS;
}
