#include "native_owner_set.h"

#include "boot_logo_skip.h"
#include "boot_object_callbacks.h"
#include "cd_file_read.h"
#include "cd_license_startup.h"
#include "cd_startup.h"
#include "display_frame.h"
#include "gpu_timeout.h"
#include "memory_card_startup.h"
#include "menu_boundary.h"
#include "model_depth_scale_capture.h"
#include "model_submit_capture.h"
#include "model_transform_capture.h"
#include "polar_push_contact.h"
#include "sprite_quad_capture.h"

namespace crashbash {

void registerNativeOwners(Core &core) {
  registerCdFileReadOverride(core);
  registerCdLicenseStartupOverride(core);
  registerCdStartupOverride(core);
  registerMemoryCardStartupOverride(core);
  registerGpuTimeoutOverrides(core);
  registerDisplayFrameOverride(core);
  registerBootObjectCallbackOverrides(core);
  registerBootLogoSkipOverride(core);
  polar::registerPolarPushContactOverride(core);
  render::registerModelDepthScaleCaptureOverride(core);
  render::registerModelTransformCaptureOverride(core);
  render::registerModelSubmitCaptureOverrides(core);
  render::registerSpriteQuadCaptureOverride(core);
  diagnostics::registerMenuBoundary(core);
}

} // namespace crashbash
