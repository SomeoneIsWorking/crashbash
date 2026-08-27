#pragma once

#include "game_runtime.h"

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

private:
  void enterProcessState(Core &core, std::uint32_t state);

  Game &game_;
  std::uint32_t activeState_ = 0;
  std::uint32_t deliveredFields_ = 0;
};

CrashBashFrameDriver &frameDriver(Core &core);

} // namespace crashbash
