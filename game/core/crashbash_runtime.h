#pragma once

#include "game_iface.h"

#include <memory>

namespace crashbash {

// Process-lifetime owner of Crash Bash's framework-facing behavior. The legacy base is bounded
// compatibility debt: generic psxport algorithms still read the title's measured program facts.
class CrashBashRuntime final : public LegacyGameRuntimeAdapter {
public:
  CrashBashRuntime();

  RenderCapabilities renderCapabilities() const override {
    return RenderCapabilities::interpolatedNative();
  }
  void registerOverrides(Game &game) override;
  void bootInit(Core &core) override;
  std::unique_ptr<FrameDriver> createFrameDriver(Game &game) override;
};

} // namespace crashbash
