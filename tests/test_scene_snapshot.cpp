#include "interpolated_scene.h"
#include "scene_snapshot.h"

#include <cstdlib>

int main() {
  using crashbash::render::isRenderableModelDraw;
  using crashbash::render::ModelDraw;
  using crashbash::render::ModelFace;
  using crashbash::render::ModelSubmitter;
  using crashbash::render::SceneSnapshotHistory;
  using crashbash::render::SpriteQuadDraw;

  crashbash::render::ModelFace sourceA{.sourceFace = 4u,
                                       .sourceVertexAddress = 0x80010000u,
                                       .sourceGroup = 2u,
                                       .sourceGroupFace = 1u,
                                       .sourceMaterial = 7u};
  crashbash::render::ModelFace sourceB = sourceA;
  if (!crashbash::render::canInterpolateModelFace(sourceA, sourceB)) {
    return EXIT_FAILURE;
  }
  sourceB.sourceVertexAddress += 24u;
  if (crashbash::render::canInterpolateModelFace(sourceA, sourceB)) {
    return EXIT_FAILURE;
  }
  sourceB = sourceA;
  sourceB.sourceVertexAddress = 0u;
  if (crashbash::render::canInterpolateModelFace(sourceA, sourceB)) {
    return EXIT_FAILURE;
  }

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
  history.record(SpriteQuadDraw{
      .sourceFunction = 0x8002992Cu,
      .sourceAddress = 0x800B0000u,
      .renderList = 0x8005F79Cu,
      .packedPosition = 0x00200010u,
      .orderingBin = 0,
      .texturePage = 0x0123u,
      .clut = 0x0456u,
  });
  if (!history.current().valid || history.current().logicFrame != 7u || history.current().models.size() != 1u ||
      history.current().spriteQuads.size() != 1u || history.current().authoredScreenPresentation ||
      history.presentable().valid) {
    return EXIT_FAILURE;
  }

  history.record(SpriteQuadDraw{.sourceFunction = 0x8001A0D8u, .authoredWorldOrder = true});
  if (!history.current().authoredScreenPresentation) {
    return EXIT_FAILURE;
  }

  history.beginFrame(8u);
  if (history.previous().valid || !history.presentable().valid || history.presentable().logicFrame != 7u ||
      history.presentable().models.size() != 1u || history.presentable().spriteQuads.size() != 2u ||
      !history.presentable().authoredScreenPresentation || !history.current().valid ||
      history.current().logicFrame != 8u || !history.current().models.empty() ||
      !history.current().spriteQuads.empty()) {
    return EXIT_FAILURE;
  }
  const SpriteQuadDraw &previousSprite = history.presentable().spriteQuads.front();
  if (previousSprite.sourceFunction != 0x8002992Cu || previousSprite.sourceAddress != 0x800B0000u ||
      previousSprite.renderList != 0x8005F79Cu || previousSprite.texturePage != 0x0123u ||
      previousSprite.clut != 0x0456u) {
    return EXIT_FAILURE;
  }
  const ModelDraw &previousDraw = history.presentable().models.front();
  if (previousDraw.depthBias != -7 || previousDraw.depthLimit != 0x1234 || previousDraw.texturedFaces != 1u ||
      previousDraw.faces.size() != 1u || !previousDraw.faces.front().textured ||
      previousDraw.faces.front().textureCoordinates[2] != 0x5060u ||
      previousDraw.faces.front().texturePage != 0x0123u || previousDraw.faces.front().clut != 0x0456u) {
    return EXIT_FAILURE;
  }

  history.record(draw);
  history.beginFrame(9u);
  if (!history.previous().valid || history.previous().logicFrame != 7u || !history.presentable().valid ||
      history.presentable().logicFrame != 8u || !history.current().valid || history.current().logicFrame != 9u ||
      history.temporalPairValid()) {
    return EXIT_FAILURE;
  }
  history.markPresented();
  if (!history.temporalPairValid()) {
    return EXIT_FAILURE;
  }
  history.markUnpresented();
  if (history.temporalPairValid()) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
