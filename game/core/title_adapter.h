#pragma once

#include "game_runtime.h"
#include "psx_exe_image.h"

#include <memory>

namespace crashbash {

// Typed psxport composition boundary. Compose the GuestExecution context with authenticated
// loaded-image lifecycle and the retained native frame/presentation owners.
class TitleAdapter final : public GameRuntime {
public:
  static constexpr RenderCapabilities titleRenderCapabilities() {
    return RenderCapabilities::interpolatedNative(FACE_ORDER_AUTHORED);
  }

  // Authenticating and loading consume one immutable byte span; no path is reopened after hashing.
  psx::cpu::PsxExeLoadResult loadExecutable(Core &core, std::span<const std::uint8_t> bytes);
  const GuestProgramImage *guestProgramImage() const override;
  const PlatformHlePlan *platformHlePlan() const override;
  const char *discEnvVar() const override;

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
