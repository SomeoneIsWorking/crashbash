#include "model_depth_scale_capture.h"

#include "core.h"
#include "crashbash_guest.h"
#include "override_registry.h"

#include <cstdint>

#ifdef CRASHBASH_HAVE_SUBSTRATE
#include "rec_decls.h"
#endif

namespace crashbash::render {
namespace {

constexpr std::uint32_t kZsf3ControlRegister = 29u;
constexpr std::uint32_t kZsf4ControlRegister = 30u;

#ifdef CRASHBASH_HAVE_SUBSTRATE
void modelDepthScaleCapture(Core *core) {
  gen_func_80033494(core);
  publishModelDepthScales(*core, gte_read_ctrl(kZsf3ControlRegister), gte_read_ctrl(kZsf4ControlRegister));
}
#endif

} // namespace

void publishModelDepthScales(Core &core, std::uint32_t zsf3, std::uint32_t zsf4) {
  core.rsub.projParams.setZsf(static_cast<std::int16_t>(zsf3), static_cast<std::int16_t>(zsf4));
}

void registerModelDepthScaleCaptureOverride() {
#ifdef CRASHBASH_HAVE_SUBSTRATE
  overrides::install(guest::kGteInitialization,
                     "CrashBash::ModelDepthScaleCapture",
                     modelDepthScaleCapture,
                     gen_func_80033494,
                     shard_set_override);
#endif
}

} // namespace crashbash::render
