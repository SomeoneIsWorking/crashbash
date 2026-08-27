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
    previous_ = std::move(current_);
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

const SceneSnapshot &SceneSnapshotHistory::previous() const {
  return previous_;
}

const SceneSnapshot &SceneSnapshotHistory::current() const {
  return current_;
}

} // namespace crashbash::render
