#include "model_face_pixel_diagnostic.h"

#include "render_queue.h"
#include "scene_snapshot.h"

#include <cstdint>
#include <lucent/log.h>

namespace crashbash::render {
namespace {

thread_local ModelFacePixelFrameCensus census;

const char *rejectionName(ModelFaceRejection rejection) {
  switch (rejection) {
  case ModelFaceRejection::None:
    return "none";
  case ModelFaceRejection::ZeroUntexturedDepth:
    return "zero-depth";
  case ModelFaceRejection::FarDepth:
    return "far-depth";
  case ModelFaceRejection::Winding:
    return "winding";
  }
  return "unknown";
}

} // namespace

bool modelFaceCoversPixel(const std::array<std::array<std::int32_t, 2>, 3> &projectedVertices,
                          std::int32_t pixelX,
                          std::int32_t pixelY) {
  // Sample the pixel center exactly while retaining the queue's integer edge predicate.
  return rq_point_in_triangle(pixelX * 2 + 1,
                              pixelY * 2 + 1,
                              projectedVertices[0][0] * 2,
                              projectedVertices[0][1] * 2,
                              projectedVertices[1][0] * 2,
                              projectedVertices[1][1] * 2,
                              projectedVertices[2][0] * 2,
                              projectedVertices[2][1] * 2);
}

void beginModelFacePixelDiagnosticFrame() {
  census = {};
}

void observeModelFaceAtPixel(const ModelDraw &draw,
                             const ModelFace &face,
                             const std::array<std::array<std::int32_t, 2>, 3> &projectedVertices,
                             const ModelFaceCoverage &coverage,
                             bool queued,
                             std::int32_t pixelX,
                             std::int32_t pixelY) {
  ++census.projectedFaces;
  if (!modelFaceCoversPixel(projectedVertices, pixelX, pixelY)) {
    return;
  }

  ++census.coveringFaces;
  switch (coverage.rejection) {
  case ModelFaceRejection::None:
    ++census.acceptedFaces;
    break;
  case ModelFaceRejection::ZeroUntexturedDepth:
    ++census.zeroDepthRejected;
    break;
  case ModelFaceRejection::FarDepth:
    ++census.farDepthRejected;
    break;
  case ModelFaceRejection::Winding:
    ++census.windingRejected;
    break;
  }
  if (queued) {
    ++census.queuedFaces;
  }
  census.witnesses.push_back({
      .object = draw.object,
      .frameCode = draw.frameCode,
      .sourceFace = face.sourceFace,
      .material = face.sourceMaterial,
      .topologyFlags = face.topologyFlags,
      .effectiveSubmitFlags = draw.objectFlags | draw.callFlags,
      .projectedVertices = projectedVertices,
      .rejection = coverage.rejection,
      .sortKey = coverage.sortKey,
      .nativeColors = face.colors,
      .retailColors = face.retailColors,
      .textured = face.textured,
      .queued = queued,
  });
}

const ModelFacePixelFrameCensus &modelFacePixelFrameCensus() {
  return census;
}

void reportModelFacePixelDiagnosticFrame(std::uint32_t frame) {
  lucent::debug(
      "crashbash-pixel-face",
      "f{} projected={} covering={} accepted={} rejected-zero={} rejected-far={} rejected-winding={} queued={}",
      frame,
      census.projectedFaces,
      census.coveringFaces,
      census.acceptedFaces,
      census.zeroDepthRejected,
      census.farDepthRejected,
      census.windingRejected,
      census.queuedFaces);
  for (const ModelFacePixelWitness &face : census.witnesses) {
    lucent::debug("crashbash-pixel-face",
                  "f{} obj={:08X} frame={:04X} face={} mat={:04X} topo={:02X} flags={:08X} "
                  "xy=[({},{}) ({},{}) ({},{})] rejection={} sort={} queued={} textured={} "
                  "native={:08X}/{:08X}/{:08X} retail={:08X}/{:08X}/{:08X}",
                  frame,
                  face.object,
                  face.frameCode,
                  face.sourceFace,
                  face.material,
                  face.topologyFlags,
                  face.effectiveSubmitFlags,
                  face.projectedVertices[0][0],
                  face.projectedVertices[0][1],
                  face.projectedVertices[1][0],
                  face.projectedVertices[1][1],
                  face.projectedVertices[2][0],
                  face.projectedVertices[2][1],
                  rejectionName(face.rejection),
                  face.sortKey,
                  face.queued,
                  face.textured,
                  face.nativeColors[0],
                  face.nativeColors[1],
                  face.nativeColors[2],
                  face.retailColors[0],
                  face.retailColors[1],
                  face.retailColors[2]);
  }
}

} // namespace crashbash::render
