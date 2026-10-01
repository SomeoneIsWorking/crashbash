#include "ui_anchor.h"

#include "gpu_vk.h"

#include <lucent/log.h>

namespace crashbash::render::ui_anchor {
namespace {

const char *channel() {
  return "uihud";
}

} // namespace

Frame frame(Core *core) {
  return Frame{.authored = gpu_vk_native_w(core), .drawn = gpu_vk_wide_engine_w(core)};
}

std::int32_t correctionAndReport(std::uint32_t logicFrame,
                                 const char *element,
                                 Anchor anchor,
                                 std::int32_t authoredX,
                                 std::int32_t authoredWidth,
                                 const Frame &frame) {
  const std::optional<std::int32_t> shift = correction(anchor, frame);
  if (!shift) {
    // A refused element is a fact about the PORT's inputs, not about the picture, so it is reported
    // with the reason and the values it refused. Drawing it uncorrected without saying so is the
    // quiet wrong answer this project keeps finding.
    lucent::warn(channel(),
                 "REFUSED f={} element={} anchor={} authored=({}, {}) reason={} frame={}->{}",
                 logicFrame,
                 element,
                 name(anchor),
                 authoredX,
                 authoredWidth,
                 refusalName(refusalFor(anchor, frame)),
                 frame.authored,
                 frame.drawn);
    return 0;
  }
  lucent::debug(channel(),
                "f={} element={} anchor={} authored=({}, {}) frame={}->{} margin={} correction={}",
                logicFrame,
                element,
                name(anchor),
                authoredX,
                authoredWidth,
                frame.authored,
                frame.drawn,
                margin(frame),
                *shift);
  return *shift;
}

} // namespace crashbash::render::ui_anchor