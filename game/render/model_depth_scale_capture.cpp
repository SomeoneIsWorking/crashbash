#include "model_depth_scale_capture.h"

#include "core.h"
#include "crashbash_guest.h"
#include "guest_execution.h"

#include <cstdint>

namespace crashbash::render {
namespace {

constexpr std::uint32_t kZsf3ControlRegister = 29u;
constexpr std::uint32_t kZsf4ControlRegister = 30u;

void modelDepthScaleCapture(Core *core) {
  runtime::callOriginal(*core, runtime::GuestImage::Resident, guest::kGteInitialization);
  publishModelDepthScales(*core, gte_read_ctrl(kZsf3ControlRegister), gte_read_ctrl(kZsf4ControlRegister));
}

} // namespace

void publishModelDepthScales(Core &core, std::uint32_t zsf3, std::uint32_t zsf4) {
  core.rsub.projParams.setZsf(static_cast<std::int16_t>(zsf3), static_cast<std::int16_t>(zsf4));
}

void registerModelDepthScaleCaptureOverride(Core &core) {
  runtime::registerNativeOverride(core,
                                  runtime::GuestImage::Resident,
                                  guest::kGteInitialization,
                                  "CrashBash::ModelDepthScaleCapture",
                                  modelDepthScaleCapture);
}

} // namespace crashbash::render
