#include "model_transform_input_diagnostic.h"

#include "core.h"

#include <array>
#include <cstdint>

namespace crashbash::render {
namespace {

thread_local ModelTransformInputCensus census;

std::array<std::uint32_t, 5> packRotation(const ModelRotation &rotation) {
  return {{
      static_cast<std::uint16_t>(rotation[0][0]) |
          (static_cast<std::uint32_t>(static_cast<std::uint16_t>(rotation[0][1])) << 16u),
      static_cast<std::uint16_t>(rotation[0][2]) |
          (static_cast<std::uint32_t>(static_cast<std::uint16_t>(rotation[1][0])) << 16u),
      static_cast<std::uint16_t>(rotation[1][1]) |
          (static_cast<std::uint32_t>(static_cast<std::uint16_t>(rotation[1][2])) << 16u),
      static_cast<std::uint16_t>(rotation[2][0]) |
          (static_cast<std::uint32_t>(static_cast<std::uint16_t>(rotation[2][1])) << 16u),
      static_cast<std::uint16_t>(rotation[2][2]),
  }};
}

void recordFirstMismatch(ModelTransformInputComparison &comparison,
                         ModelTransformInputMismatch mismatch,
                         std::uint32_t index,
                         std::uint32_t expected,
                         std::uint32_t actual) {
  if (comparison.firstMismatch != ModelTransformInputMismatch::None) {
    return;
  }
  comparison.firstMismatch = mismatch;
  comparison.firstMismatchIndex = index;
  comparison.firstExpected = expected;
  comparison.firstActual = actual;
}

InstalledModelTransformInputs readInstalledInputs() {
  InstalledModelTransformInputs installed;
  for (std::uint32_t word = 0; word < installed.rotationWords.size(); ++word) {
    installed.rotationWords[word] = gte_read_ctrl(word);
  }
  for (std::uint32_t component = 0; component < installed.translation.size(); ++component) {
    installed.translation[component] = static_cast<std::int32_t>(gte_read_ctrl(5u + component));
  }
  installed.projectionX = static_cast<std::int32_t>(gte_read_ctrl(24u));
  installed.projectionY = static_cast<std::int32_t>(gte_read_ctrl(25u));
  installed.projectionDistance = static_cast<std::uint16_t>(gte_read_ctrl(26u));
  return installed;
}

} // namespace

ModelTransformInputComparison compareModelTransformInputs(const ModelTransform &expected,
                                                          const InstalledModelTransformInputs &installed) {
  ModelTransformInputComparison comparison{
      .rotationMatches = true,
      .translationMatches = true,
      .projectionMatches = true,
  };

  const std::array<std::uint32_t, 5> expectedRotation = packRotation(expected.rotation);
  for (std::uint32_t word = 0; word < expectedRotation.size(); ++word) {
    if (expectedRotation[word] == installed.rotationWords[word]) {
      continue;
    }
    comparison.rotationMatches = false;
    recordFirstMismatch(
        comparison, ModelTransformInputMismatch::Rotation, word, expectedRotation[word], installed.rotationWords[word]);
  }

  for (std::uint32_t component = 0; component < expected.translation.size(); ++component) {
    const std::uint32_t expectedWord = static_cast<std::uint32_t>(expected.translation[component]);
    const std::uint32_t installedWord = static_cast<std::uint32_t>(installed.translation[component]);
    if (expectedWord == installedWord) {
      continue;
    }
    comparison.translationMatches = false;
    recordFirstMismatch(comparison, ModelTransformInputMismatch::Translation, component, expectedWord, installedWord);
  }

  if (expected.projectionX != installed.projectionX) {
    comparison.projectionMatches = false;
    recordFirstMismatch(comparison,
                        ModelTransformInputMismatch::ProjectionX,
                        0,
                        static_cast<std::uint32_t>(expected.projectionX),
                        static_cast<std::uint32_t>(installed.projectionX));
  }
  if (expected.projectionY != installed.projectionY) {
    comparison.projectionMatches = false;
    recordFirstMismatch(comparison,
                        ModelTransformInputMismatch::ProjectionY,
                        0,
                        static_cast<std::uint32_t>(expected.projectionY),
                        static_cast<std::uint32_t>(installed.projectionY));
  }
  if (expected.projectionDistance != installed.projectionDistance) {
    comparison.projectionMatches = false;
    recordFirstMismatch(comparison,
                        ModelTransformInputMismatch::ProjectionDistance,
                        0,
                        expected.projectionDistance,
                        installed.projectionDistance);
  }
  return comparison;
}

void observeInstalledModelTransformInputs(const ModelTransform &expected, ModelSubmitter submitter) {
  const ModelTransformInputComparison comparison = compareModelTransformInputs(expected, readInstalledInputs());
  std::uint32_t &compared = submitter == ModelSubmitter::Standard ? census.standardCompared : census.alternateCompared;
  std::uint32_t &matched = submitter == ModelSubmitter::Standard ? census.standardMatched : census.alternateMatched;
  ++compared;
  matched += comparison.matches() ? 1u : 0u;
  census.rotationMismatches += comparison.rotationMatches ? 0u : 1u;
  census.translationMismatches += comparison.translationMatches ? 0u : 1u;
  census.projectionMismatches += comparison.projectionMatches ? 0u : 1u;
  if (!comparison.matches() && census.firstMismatch.firstMismatch == ModelTransformInputMismatch::None) {
    census.firstMismatch = comparison;
  }
}

const ModelTransformInputCensus &modelTransformInputCensus() {
  return census;
}

} // namespace crashbash::render
