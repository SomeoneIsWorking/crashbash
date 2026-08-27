#include "model_face_coverage.h"

#include <array>
#include <cstdlib>

namespace {

using crashbash::render::classifyFixedModelFace;
using crashbash::render::ModelFaceRejection;
using crashbash::render::ProjectedFaceVertex;

constexpr std::array<ProjectedFaceVertex, 3> kFrontFacing{{
    {.x = 0, .y = 0, .depth = 0},
    {.x = 2, .y = 0, .depth = 0},
    {.x = 0, .y = 2, .depth = 20},
}};

bool check(bool condition) {
  return condition;
}

} // namespace

int main() {
  const auto accepted = classifyFixedModelFace(kFrontFacing, false, 0u, 4, 100);
  if (!check(accepted.accepted() && accepted.sortKey == 12u)) {
    return EXIT_FAILURE;
  }

  auto zeroThird = kFrontFacing;
  zeroThird[2].depth = 0;
  if (!check(classifyFixedModelFace(zeroThird, false, 2u, 4, 100).rejection ==
             ModelFaceRejection::ZeroUntexturedDepth)) {
    return EXIT_FAILURE;
  }
  if (!check(classifyFixedModelFace(zeroThird, true, 2u, 4, 100).accepted())) {
    return EXIT_FAILURE;
  }

  if (!check(classifyFixedModelFace(kFrontFacing, false, 2u, 0, 10).rejection == ModelFaceRejection::FarDepth)) {
    return EXIT_FAILURE;
  }
  if (!check(classifyFixedModelFace(kFrontFacing, false, 0u, 0, 100).accepted())) {
    return EXIT_FAILURE;
  }
  if (!check(classifyFixedModelFace(kFrontFacing, false, 1u, 0, 100).rejection == ModelFaceRejection::Winding)) {
    return EXIT_FAILURE;
  }
  if (!check(classifyFixedModelFace(kFrontFacing, false, 2u, 0, 100).accepted())) {
    return EXIT_FAILURE;
  }

  const auto wrapped = classifyFixedModelFace(kFrontFacing, false, 2u, -21, 100);
  if (!check(wrapped.rejection == ModelFaceRejection::FarDepth && wrapped.sortKey == 0x7FFFFFFFu)) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
