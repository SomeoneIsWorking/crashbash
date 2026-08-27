#pragma once

#include "game_runtime.h"
#include "scene_snapshot.h"

#include <cstdint>

class Core;
class Game;

namespace crashbash {

class CrashBashFrameDriver final : public FrameDriver {
public:
  explicit CrashBashFrameDriver(Game &game);

  void stepFrame(Core &core, std::uint32_t frame) override;

  // Called at the exact point native DisplayFrame removes retail VSync(fields). This owns field
  // callbacks and audio; presentation remains the single commit at the frame-driver tail.
  void deliverDisplayFields(Core &core, std::uint32_t fields);

  render::SceneSnapshotHistory &sceneSnapshots();

private:
  void enterProcessState(Core &core, std::uint32_t state);

  // Progress reporting. The interesting event is a process-state CHANGE, and a run that never
  // changes state is precisely the failure worth seeing — so the boring case (sitting in one state)
  // is what gets capped, never the transitions. reportProgress() is called unconditionally at the
  // end of every frame and always prints something for a state it has dwelled in, so "no output"
  // cannot be confused with "no state machine ran".
  void reportProgress(Core &core, std::uint32_t frame);

  Game &game_;
  std::uint32_t activeState_ = 0;
  std::uint32_t deliveredFields_ = 0;
  std::uint32_t stateEntries_ = 0;
  std::uint32_t dwellFrames_ = 0;
  std::uint32_t dwellReports_ = 0;
  std::uint32_t updateFn_ = 0;
  std::uint32_t presentFn_ = 0;
  std::uint32_t appMode_ = 0;
  std::uint32_t appModeChanges_ = 0;
  render::SceneSnapshotHistory sceneSnapshots_;
};

CrashBashFrameDriver &frameDriver(Core &core);

} // namespace crashbash
