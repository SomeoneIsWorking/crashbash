#pragma once

#include "game_runtime.h"

#include <memory>

namespace crashbash {

// Typed psxport composition boundary. Its implementation is deliberately absent until Lightrec and
// loaded-image lifecycle binding are available; the build must not substitute the legacy adapter.
class TitleAdapter final : public GameRuntime {
public:
  static constexpr RenderCapabilities titleRenderCapabilities() {
    return RenderCapabilities::interpolatedNative(FACE_ORDER_AUTHORED);
  }

  void *createContext(Core &core) override;
  void destroyContext(void *context) override;
  void registerOverrides(Game &game) override;
  void bootInit(Core &core) override;
  RenderCapabilities renderCapabilities() const override;
  bool guestVramIsPicture(const Game &game) const override;
  std::unique_ptr<TemporalFramePresentation> createTemporalFramePresentation(Game &game) override;
  std::unique_ptr<FrameDriver> createFrameDriver(Game &game) override;
};

} // namespace crashbash
