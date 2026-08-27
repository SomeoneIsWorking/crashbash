#include "scene_snapshot.h"

#include <cstdlib>

int main() {
  using crashbash::render::isRenderableModelDraw;
  using crashbash::render::ModelDraw;
  using crashbash::render::ModelFace;
  using crashbash::render::ModelSubmitter;
  using crashbash::render::SceneSnapshotHistory;

  SceneSnapshotHistory history;
  history.beginFrame(7u);
  const ModelDraw draw{
      .submitter = ModelSubmitter::Standard,
      .object = 0x80090000u,
      .objectFlags = 0x8000u,
      .matrix = 0x80090100u,
      .translation = 0x80090114u,
      .callFlags = 3u,
      .modelAsset = 0x800A0000u,
      .modelData = 0x800A1000u,
      .frameCode = 0x2001u,
      .depthBias = -7,
      .depthLimit = 0x1234,
      .faces = {ModelFace{
          .textureCoordinates = {0x1020u, 0x3040u, 0x5060u},
          .texturePage = 0x0123u,
          .clut = 0x0456u,
          .textured = true,
      }},
      .texturedFaces = 1u,
  };
  if (!isRenderableModelDraw(draw)) {
    return EXIT_FAILURE;
  }
  ModelDraw sentinelFrame = draw;
  sentinelFrame.frameCode = 0xFFFFu;
  if (isRenderableModelDraw(sentinelFrame)) {
    return EXIT_FAILURE;
  }
  history.record(draw);
  if (!history.current().valid || history.current().logicFrame != 7u || history.current().models.size() != 1u ||
      history.previous().valid) {
    return EXIT_FAILURE;
  }

  history.beginFrame(8u);
  if (!history.previous().valid || history.previous().logicFrame != 7u || history.previous().models.size() != 1u ||
      !history.current().valid || history.current().logicFrame != 8u || !history.current().models.empty()) {
    return EXIT_FAILURE;
  }
  const ModelDraw &previousDraw = history.previous().models.front();
  if (previousDraw.depthBias != -7 || previousDraw.depthLimit != 0x1234 || previousDraw.texturedFaces != 1u ||
      previousDraw.faces.size() != 1u || !previousDraw.faces.front().textured ||
      previousDraw.faces.front().textureCoordinates[2] != 0x5060u ||
      previousDraw.faces.front().texturePage != 0x0123u || previousDraw.faces.front().clut != 0x0456u) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
