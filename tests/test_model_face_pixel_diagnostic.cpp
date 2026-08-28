#include "model_face_pixel_diagnostic.h"
#include "scene_snapshot.h"

#include <array>
#include <cstdint>
#include <cstdio>

int main() {
  using crashbash::render::beginModelFacePixelDiagnosticFrame;
  using crashbash::render::ModelDraw;
  using crashbash::render::ModelFace;
  using crashbash::render::ModelFaceCoverage;
  using crashbash::render::modelFaceCoversPixel;
  using crashbash::render::modelFacePixelFrameCensus;
  using crashbash::render::ModelFaceRejection;
  using crashbash::render::observeModelFaceAtPixel;

  int failures = 0;
  const auto check = [&failures](bool condition, const char *message) {
    if (!condition) {
      std::fprintf(stderr, "FAIL: %s\n", message);
      ++failures;
    }
  };

  const std::array<std::array<std::int32_t, 2>, 3> covering{{{{34, 114}}, {{38, 114}}, {{34, 118}}}};
  const std::array<std::array<std::int32_t, 2>, 3> outside{{{{0, 0}}, {{1, 0}}, {{0, 1}}}};
  const std::array<std::array<std::int32_t, 2>, 3> degenerate{{{{0, 0}}, {{0, 0}}, {{0, 0}}}};
  check(modelFaceCoversPixel(covering, 35, 115), "pixel center inside triangle is covered");
  check(!modelFaceCoversPixel(outside, 35, 115), "outside pixel is not covered");
  check(!modelFaceCoversPixel(degenerate, 35, 115), "degenerate triangle never covers");

  ModelDraw draw{
      .object = 0x800A0C74u,
      .objectFlags = 0x100u,
      .callFlags = 0x20u,
      .frameCode = 0x200Bu,
  };
  ModelFace face{
      .colors = {0x11u, 0x22u, 0x33u},
      .textured = false,
      .retailColors = {0x44u, 0x55u, 0x66u},
      .sourceFace = 17u,
      .sourceMaterial = 0x1234u,
      .topologyFlags = 0x42u,
  };
  beginModelFacePixelDiagnosticFrame();
  observeModelFaceAtPixel(draw, face, outside, {.rejection = ModelFaceRejection::None, .sortKey = 10u}, true, 35, 115);
  observeModelFaceAtPixel(
      draw, face, covering, {.rejection = ModelFaceRejection::Winding, .sortKey = 20u}, false, 35, 115);
  face.sourceFace = 18u;
  observeModelFaceAtPixel(draw, face, covering, {.rejection = ModelFaceRejection::None, .sortKey = 30u}, true, 35, 115);

  const auto &census = modelFacePixelFrameCensus();
  check(census.projectedFaces == 3, "all pre-filter faces contribute to denominator");
  check(census.coveringFaces == 2, "both pre-filter covering faces are retained");
  check(census.windingRejected == 1, "winding rejection is attributed");
  check(census.acceptedFaces == 1, "accepted covering face is counted");
  check(census.queuedFaces == 1, "queue reach is counted independently");
  check(census.witnesses.size() == 2, "every covering witness is retained");
  check(census.witnesses[0].sourceFace == 17u && !census.witnesses[0].queued,
        "rejected face identity and queue result are retained");
  check(census.witnesses[1].sourceFace == 18u && census.witnesses[1].queued,
        "accepted face identity and queue result are retained");
  check(census.witnesses[1].effectiveSubmitFlags == 0x120u, "effective flags are source-owned");

  if (failures != 0) {
    return 1;
  }
  std::puts("model face pixel diagnostic: all checks passed");
  return 0;
}
