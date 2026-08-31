#include "crashbash_runtime.h"

#include "boot_object_callbacks.h"
#include "cd_file_read.h"
#include "cd_license_startup.h"
#include "cd_startup.h"
#include "core.h"
#include "crashbash_boot.h"
#include "crashbash_frame_driver.h"
#include "display_frame.h"
#include "gpu_timeout.h"
#include "interpolated_scene.h"
#include "legacy_game_interface.h"
#include "memcard.h"
#include "memory_card_startup.h"
#include "menu_boundary.h"
#include "model_depth_scale_capture.h"
#include "model_submit_capture.h"
#include "model_transform_capture.h"
#include "polar_push_contact.h"
#include "sprite_quad_capture.h"

#include <memory>

#include <lucent/log.h>

namespace crashbash {

CrashBashRuntime::CrashBashRuntime() : LegacyGameRuntimeAdapter(legacy::measuredConfig, legacy::compatibilityHooks) {}

void CrashBashRuntime::registerOverrides(Game &game) {
  // The card is a BIOS DEVICE before it is a file. Crash Bash's stock libmcrd resolves "bu00:*" by
  // walking the kernel device table at 0x150/0x154 ITSELF (0x8004799C) instead of calling a BIOS
  // vector, and its "device not found" answer is indistinguishable from "request queued": it returns
  // 0, and the caller 0x8003A554 then waits in 0x800476EC for a completion callback that was never
  // started. Without this the port watchdogged there with no card operation ever attempted.
  card_overrides_init(&game);
  registerCdFileReadOverride();
  registerCdLicenseStartupOverride();
  registerCdStartupOverride();
  registerMemoryCardStartupOverride();
  registerGpuTimeoutOverrides();
  registerDisplayFrameOverride();
  registerBootObjectCallbackOverrides();
  polar::registerPolarPushContactOverride();
  render::registerModelDepthScaleCaptureOverride();
  render::registerModelTransformCaptureOverride();
  render::registerModelSubmitCaptureOverrides();
  render::registerSpriteQuadCaptureOverride();
  diagnostics::registerMenuBoundary();
}

void CrashBashRuntime::bootInit(Core &core) {
  lucent::info("boot", "executing finite Crash Bash boot prefix; native FrameDriver owns repetition");
  runBootPrefix(core);
}

std::unique_ptr<TemporalFramePresentation> CrashBashRuntime::createTemporalFramePresentation(Game &game) {
  return std::make_unique<render::InterpolatedScenePresentation>(game);
}

std::unique_ptr<FrameDriver> CrashBashRuntime::createFrameDriver(Game &game) {
  return std::make_unique<CrashBashFrameDriver>(game);
}

} // namespace crashbash
