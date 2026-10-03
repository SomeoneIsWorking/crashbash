#pragma once

#include "scene_snapshot.h"

#include <array>
#include <cstdint>

namespace crashbash::render {

struct InstalledModelTransformInputs {
  std::array<std::uint32_t, 5> rotationWords{};
  std::array<std::int32_t, 3> translation{};
  std::int32_t projectionX = 0;
  std::int32_t projectionY = 0;
  std::uint16_t projectionDistance = 0;
};

enum class ModelTransformInputMismatch : std::uint8_t {
  None,
  Rotation,
  Translation,
  ProjectionX,
  ProjectionY,
  ProjectionDistance,
};

struct ModelTransformInputComparison {
  bool rotationMatches = false;
  bool translationMatches = false;
  bool projectionMatches = false;
  ModelTransformInputMismatch firstMismatch = ModelTransformInputMismatch::None;
  std::uint32_t firstMismatchIndex = 0;
  std::uint32_t firstExpected = 0;
  std::uint32_t firstActual = 0;

  bool matches() const {
    return rotationMatches && translationMatches && projectionMatches;
  }
};

struct ModelTransformInputCensus {
  std::uint32_t standardCompared = 0;
  std::uint32_t standardMatched = 0;
  std::uint32_t alternateCompared = 0;
  std::uint32_t alternateMatched = 0;
  std::uint32_t rotationMismatches = 0;
  std::uint32_t translationMismatches = 0;
  std::uint32_t projectionMismatches = 0;
  ModelTransformInputComparison firstMismatch;
};

ModelTransformInputComparison compareModelTransformInputs(const ModelTransform &expected,
                                                          const InstalledModelTransformInputs &installed);

// Samples the GTE control inputs installed by the retained retail super and records whether they
// agree with the title-owned source capture. Diagnostic only: it never supplies product rendering
// state, and one instance belongs to the frame driver whose run it measures.
class TransformInputDiagnostic {
public:
  void observe(const ModelTransform &expected, ModelSubmitter submitter);

  const ModelTransformInputCensus &census() const {
    return census_;
  }

private:
  ModelTransformInputCensus census_{};
};

} // namespace crashbash::render
