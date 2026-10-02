#include "render_viewport.h"

namespace crashbash::render {
namespace {

constexpr std::int32_t kAuthoredWidth = 0x280;

std::int32_t authoredToPixelsX(std::int32_t authored, std::int16_t displayScale) {
  return authored * displayScale / kAuthoredWidth;
}

} // namespace

ViewportClip viewportClip(const ViewportRecord &record, std::int16_t displayScale) {
  const std::int32_t x = authoredToPixelsX(record.left, displayScale);
  const std::int32_t y = record.top / 2;
  const std::int32_t width = authoredToPixelsX(record.right - record.left, displayScale);
  const std::int32_t height = (record.bottom - record.top) / 2;
  return {.x0 = x, .y0 = y, .x1 = x + width - 1, .y1 = y + height - 1};
}

ViewportColumns widenedViewportColumns(const ViewportClip &clip, std::int32_t nativeWidth, std::int32_t margin) {
  return {
      .x0 = clip.x0 <= 0 ? clip.x0 : clip.x0 + margin,
      .x1 = clip.x1 >= nativeWidth - 1 ? clip.x1 + 2 * margin : clip.x1 + margin,
  };
}

} // namespace crashbash::render
