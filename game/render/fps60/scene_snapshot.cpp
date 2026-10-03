#include "scene_snapshot.h"

#include <cstdlib>
#include <utility>

namespace crashbash::render {

bool isRenderableModelDraw(const ModelDraw &draw) {
  return draw.object != 0 && (draw.objectFlags & 0x8000u) != 0 && draw.modelAsset != 0 && draw.modelData != 0 &&
         draw.frameCode != 0 && draw.frameCode != 0xFFFFu;
}

void SceneSnapshotHistory::beginFrame(std::uint32_t logicFrame) {
  if (current_.valid && logicFrame <= current_.logicFrame) {
    std::abort();
  }
  if (current_.valid) {
    previous_ = std::move(presentable_);
    presentable_ = std::move(current_);
  }
  current_ = SceneSnapshot{.logicFrame = logicFrame, .valid = true};
}

ModelDraw &SceneSnapshotHistory::record(ModelDraw draw) {
  if (!current_.valid || !isRenderableModelDraw(draw)) {
    std::abort();
  }
  current_.models.push_back(std::move(draw));
  return current_.models.back();
}

SpriteQuadDraw &SceneSnapshotHistory::record(SpriteQuadDraw draw) {
  if (!current_.valid) {
    std::abort();
  }
  current_.authoredScreenPresentation |= draw.centered4x3Composition;
  current_.spriteQuads.push_back(draw);
  return current_.spriteQuads.back();
}

SceneSnapshot &SceneSnapshotHistory::presentable() {
  return presentable_;
}

const SceneSnapshot &SceneSnapshotHistory::presentable() const {
  return presentable_;
}

const SceneSnapshot &SceneSnapshotHistory::previous() const {
  return previous_;
}

const SceneSnapshot &SceneSnapshotHistory::current() const {
  return current_;
}

bool SceneSnapshotHistory::temporalPairValid() const {
  return temporalContinuous_ && previous_.valid && presentable_.valid;
}

void SceneSnapshotHistory::markPresented() {
  temporalContinuous_ = true;
}

void SceneSnapshotHistory::markUnpresented() {
  temporalContinuous_ = false;
}

} // namespace crashbash::render
