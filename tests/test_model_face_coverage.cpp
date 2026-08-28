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

int main() {
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
