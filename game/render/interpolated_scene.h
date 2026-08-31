#pragma once

#include "frame_presenter.h"
#include "scene_snapshot.h"

#include <memory>
#include <vector>

class Game;
class RenderQueue;

namespace crashbash::render {

ModelTransform interpolateModelTransform(const ModelTransform &previous, const ModelTransform &current, float alpha);
ModelVertex interpolateModelVertex(const ModelVertex &previous, const ModelVertex &current, float alpha);
inline bool canInterpolateModelFace(const ModelFace &previous, const ModelFace &current) {
  return previous.sourceVertexAddress != 0 && current.sourceVertexAddress != 0 &&
         previous.sourceFace == current.sourceFace && previous.sourceGroup == current.sourceGroup &&
         previous.sourceGroupFace == current.sourceGroupFace && previous.sourceMaterial == current.sourceMaterial &&
         previous.sourceVertexAddress == current.sourceVertexAddress &&
         previous.topologyFlags == current.topologyFlags && previous.textured == current.textured;
}

// Crash Bash owns its temporal input as decoded scene snapshots. The framework still owns the host
// present/pacing operations; this decorator only replaces the native model block with a midpoint
// rebuilt from the two completed title snapshots. Screen-space and not-yet-native layers remain the
// captured current frame, so enabling 60 Hz never advances simulation or reads guest state at present.
class InterpolatedScenePresentation final : public TemporalFramePresentation {
public:
  explicit InterpolatedScenePresentation(Game &game);
  ~InterpolatedScenePresentation() override;

  void present(FramePresentationBackend &backend, Core &core, CapturedFrameView frame, int guestFields) override;

private:
  bool active(const Core &core) const;
  long buildMidpoint(Core &core, const SceneSnapshot &previous, const SceneSnapshot &current);
  void buildPresentationStream(CapturedFrameView frame, bool replaceModels);

  Game &game_;
  std::unique_ptr<RenderQueue> midpointQueue_;
  std::vector<const RqItem *> presentationStream_;
};

} // namespace crashbash::render
