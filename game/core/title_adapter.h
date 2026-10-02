#pragma once

#include "game_runtime.h"
#include "psx_exe_image.h"

#include <memory>

namespace crashbash {
namespace diagnostics {
// A member reference needs no definition: this header hands back the Core's ledger, it does not
// know what a ledger contains, and including it here would make every consumer of this boundary
// depend on the diagnostics owner for no reason.
class RunLedger;
} // namespace diagnostics

// Typed psxport composition boundary. Compose the GuestExecution context with authenticated
// loaded-image lifecycle and the retained native frame/presentation owners.
class TitleAdapter final : public GameRuntime {
public:
  static constexpr RenderCapabilities titleRenderCapabilities() {
    return RenderCapabilities::interpolatedNative(FACE_ORDER_AUTHORED);
  }

  // Authenticating and loading consume one immutable byte span; no path is reopened after hashing.
  psx::cpu::PsxExeLoadResult loadExecutable(Core &core, std::span<const std::uint8_t> bytes);

  // The whole-run ledger this Core's own title context owns. The product reaches it here, through
  // the object it already composed, so the run that closes the ledger is the run that filled it and
  // there is no accessor for anyone else to use.
  diagnostics::RunLedger &runLedger(Core &core) const;
  const GuestProgramImage *guestProgramImage() const override;
  const PlatformHlePlan *platformHlePlan() const override;
  // The guest globals naming the two heap packet pools. See the definition for the addresses and
  // why a title declares them at all.
  const GuestPacketPoolWindows *guestPacketPoolWindows() const override;
  const char *discEnvVar() const override;
  // The Core's own image-identity state (runtime::ImageIdentityState), owned by its title context.
  psx::state::NativeStatePort *nativeState(Core &core) const override;

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
