#include "model_transform_input_diagnostic.h"

#include <cassert>
#include <cstdint>

namespace {

crashbash::render::ModelTransform makeTransform() {
  return {
      .rotation = {{{0x1000, -2, 3}, {4, 0x0800, -6}, {7, 8, 0x0400}}},
      .translation = {0x10203040, -3, 0x7FFFFFFF},
      .projectionX = 160 << 16,
      .projectionY = 120 << 16,
      .projectionDistance = 256,
      .valid = true,
  };
}

crashbash::render::InstalledModelTransformInputs makeInstalledInputs() {
  return {
      .rotationWords = {{0xFFFE1000u, 0x00040003u, 0xFFFA0800u, 0x00080007u, 0x00000400u}},
      .translation = {0x10203040, -3, 0x7FFFFFFF},
      .projectionX = 160 << 16,
      .projectionY = 120 << 16,
      .projectionDistance = 256,
  };
}

} // namespace

int main() {
  using crashbash::render::compareModelTransformInputs;
  using crashbash::render::ModelTransformInputMismatch;

  const crashbash::render::ModelTransform transform = makeTransform();
  crashbash::render::InstalledModelTransformInputs installed = makeInstalledInputs();
  const auto matching = compareModelTransformInputs(transform, installed);
  assert(matching.matches());
  assert(matching.firstMismatch == ModelTransformInputMismatch::None);

  installed.rotationWords[2] ^= 1u;
  auto mismatch = compareModelTransformInputs(transform, installed);
  assert(!mismatch.matches());
  assert(!mismatch.rotationMatches && mismatch.translationMatches && mismatch.projectionMatches);
  assert(mismatch.firstMismatch == ModelTransformInputMismatch::Rotation);
  assert(mismatch.firstMismatchIndex == 2u);
  assert(mismatch.firstExpected == 0xFFFA0800u);
  assert(mismatch.firstActual == 0xFFFA0801u);

  installed = makeInstalledInputs();
  ++installed.translation[1];
  mismatch = compareModelTransformInputs(transform, installed);
  assert(mismatch.rotationMatches && !mismatch.translationMatches && mismatch.projectionMatches);
  assert(mismatch.firstMismatch == ModelTransformInputMismatch::Translation);
  assert(mismatch.firstMismatchIndex == 1u);

  installed = makeInstalledInputs();
  ++installed.projectionDistance;
  mismatch = compareModelTransformInputs(transform, installed);
  assert(mismatch.rotationMatches && mismatch.translationMatches && !mismatch.projectionMatches);
  assert(mismatch.firstMismatch == ModelTransformInputMismatch::ProjectionDistance);
  return 0;
}
