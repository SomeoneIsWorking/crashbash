#pragma once

#include "model_face_coverage.h"

#include <array>
#include <cstdint>
#include <vector>

namespace crashbash::render {

struct ModelDraw;
struct ModelFace;

struct ModelFacePixelWitness {
  std::uint32_t object = 0;
  std::uint16_t frameCode = 0;
  std::uint32_t sourceFace = 0;
  std::uint16_t material = 0;
  std::uint8_t topologyFlags = 0;
  std::uint32_t effectiveSubmitFlags = 0;
  std::array<std::array<std::int32_t, 2>, 3> projectedVertices{};
  ModelFaceRejection rejection = ModelFaceRejection::None;
  std::uint32_t sortKey = 0;
  std::array<std::uint32_t, 3> nativeColors{};
  std::array<std::uint32_t, 3> retailColors{};
  bool textured = false;
  bool queued = false;
};

struct ModelFacePixelFrameCensus {
  std::uint32_t projectedFaces = 0;
  std::uint32_t coveringFaces = 0;
  std::uint32_t acceptedFaces = 0;
  std::uint32_t zeroDepthRejected = 0;
  std::uint32_t farDepthRejected = 0;
  std::uint32_t windingRejected = 0;
  std::uint32_t queuedFaces = 0;
  std::vector<ModelFacePixelWitness> witnesses;
};

bool modelFaceCoversPixel(const std::array<std::array<std::int32_t, 2>, 3> &projectedVertices,
                          std::int32_t pixelX,
                          std::int32_t pixelY);
void beginModelFacePixelDiagnosticFrame();
void observeModelFaceAtPixel(const ModelDraw &draw,
                             const ModelFace &face,
                             const std::array<std::array<std::int32_t, 2>, 3> &projectedVertices,
                             const ModelFaceCoverage &coverage,
                             bool queued,
                             std::int32_t pixelX,
                             std::int32_t pixelY);
const ModelFacePixelFrameCensus &modelFacePixelFrameCensus();
void reportModelFacePixelDiagnosticFrame(std::uint32_t frame);

} // namespace crashbash::render
