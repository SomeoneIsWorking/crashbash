#pragma once

#include "scene_snapshot.h"

#include <array>
#include <cstddef>
#include <cstdint>

class Core;

namespace crashbash::render {

// The guest model submitters still run for their game-state side effects.  Their delayed OT packets
// may be suppressed only after their native replacement has a complete source snapshot; otherwise a
// missing capture would turn into missing geometry.  This ledger is deliberately title-local: it
// describes Crash Bash's two model submitters, not a generic packet-filter policy.
class ModelPacketOwnership {
public:
  void beginLogicFrame(std::uint32_t logicFrame) {
    if (valid_ && logicFrame == logicFrame_) {
      return;
    }
    logicFrame_ = logicFrame;
    valid_ = true;
    submitters_ = {};
  }

  void noteNativeSnapshot(ModelSubmitter submitter, bool complete) {
    SubmitterState &state = submitters_[index(submitter)];
    state.seen = true;
    state.complete = state.complete && complete;
  }

  bool completeFor(std::uint32_t logicFrame, ModelSubmitter submitter) const {
    const SubmitterState &state = submitters_[index(submitter)];
    return valid_ && logicFrame_ == logicFrame && state.seen && state.complete;
  }

private:
  struct SubmitterState {
    bool seen = false;
    bool complete = true;
  };

  static constexpr std::size_t index(ModelSubmitter submitter) {
    return submitter == ModelSubmitter::Standard ? 0u : 1u;
  }

  std::uint32_t logicFrame_ = 0;
  bool valid_ = false;
  std::array<SubmitterState, 2> submitters_{};
};

// A rendered snapshot is a valid replacement for one submitter only when it contains that submitter's
// decoded, transformed source faces.  An empty/missing snapshot leaves its retail packets visible.
inline bool nativeSnapshotCompleteForSubmitter(const SceneSnapshot &snapshot, ModelSubmitter submitter) {
  if (!snapshot.valid) {
    return false;
  }
  bool found = false;
  for (const ModelDraw &draw : snapshot.models) {
    if (draw.submitter != submitter) {
      continue;
    }
    found = true;
    if (!draw.transform.valid || draw.faces.empty()) {
      return false;
    }
  }
  return found;
}

// Enables suppression only for packet spans that were tagged while a complete native model submitter
// super-call ran.  Its lifetime is the delayed OT walk: the guest packets remain written, and unrelated
// guest primitives retain no matching owner tag and therefore remain visible.
class ModelPacketSuppressionScope {
public:
  ModelPacketSuppressionScope(Core &core, const SceneSnapshot &renderedSnapshot);
  ~ModelPacketSuppressionScope();
  ModelPacketSuppressionScope(const ModelPacketSuppressionScope &) = delete;
  ModelPacketSuppressionScope &operator=(const ModelPacketSuppressionScope &) = delete;

private:
  Core *core_ = nullptr;
  std::array<bool, 2> active_{};
};

void registerModelSubmitCaptureOverrides();

} // namespace crashbash::render
