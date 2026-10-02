// test_render_viewport.cpp — the guest viewport's clip arithmetic and its widened columns.
//
// The records are the SHIPPING ones, read off a live CHOOSE LEVEL frame at the viewport 0x800569A8
// published when each model was submitted: the full-screen menu viewport and two of the arena previews.
// The display scale is the 512-pixel framebuffer's.
#include "render_viewport.h"

#include <cstdio>
#include <cstdlib>
#include <initializer_list>

namespace {

using crashbash::render::ViewportClip;
using crashbash::render::viewportClip;
using crashbash::render::ViewportColumns;
using crashbash::render::ViewportRecord;
using crashbash::render::widenedViewportColumns;

constexpr std::int16_t kDisplayScale = 512;
constexpr std::int32_t kNativeWidth = 512;
constexpr std::int32_t kWideMargin = 86;

constexpr ViewportRecord kFullScreen{.left = 0, .top = 0, .right = 640, .bottom = 480};
constexpr ViewportRecord kPreviewPanel{.left = 460, .top = 336, .right = 604, .bottom = 432};
constexpr ViewportRecord kArenaPanel{.left = 40, .top = 238, .right = 410, .bottom = 432};

int failures = 0;

void expect(bool condition, const char *what) {
  if (!condition) {
    ++failures;
    std::fprintf(stderr, "[render-viewport] FAILED: %s\n", what);
  }
}

bool same(const ViewportClip &clip, std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1) {
  return clip.x0 == x0 && clip.y0 == y0 && clip.x1 == x1 && clip.y1 == y1;
}

bool same(const ViewportColumns &columns, std::int32_t x0, std::int32_t x1) {
  return columns.x0 == x0 && columns.x1 == x1;
}

void clipFollowsTheGuestArithmetic() {
  expect(same(viewportClip(kFullScreen, kDisplayScale), 0, 0, 511, 239), "full-screen viewport is the framebuffer");
  // 460 * 512 / 640 = 368; (604 - 460) * 512 / 640 = 115.2 -> 115 columns; 336 / 2 = 168; 96 / 2 = 48 rows.
  expect(same(viewportClip(kPreviewPanel, kDisplayScale), 368, 168, 482, 215), "preview panel clip");
  // 40 * 512 / 640 = 32; 370 * 512 / 640 = 296 columns; 238 / 2 = 119; 194 / 2 = 97 rows.
  expect(same(viewportClip(kArenaPanel, kDisplayScale), 32, 119, 327, 215), "arena panel clip");
  // Truncation toward zero, as the guest's divisions do, for an authored rectangle left of the frame.
  expect(same(viewportClip({.left = -3, .top = -3, .right = 3, .bottom = 3}, kDisplayScale), -2, -1, 1, 1),
         "negative coordinates truncate toward zero");
}

void fourThreeIsIdentity() {
  for (const ViewportRecord &record : {kFullScreen, kPreviewPanel, kArenaPanel}) {
    const ViewportClip clip = viewportClip(record, kDisplayScale);
    expect(same(widenedViewportColumns(clip, kNativeWidth, 0), clip.x0, clip.x1), "4:3 columns are the clip's");
  }
}

void wideFrameEdgesMoveOutAndPanelsMoveWithTheComposition() {
  expect(same(widenedViewportColumns(viewportClip(kFullScreen, kDisplayScale), kNativeWidth, kWideMargin), 0, 683),
         "full-screen viewport covers the widened frame");
  expect(same(widenedViewportColumns(viewportClip(kPreviewPanel, kDisplayScale), kNativeWidth, kWideMargin), 454, 568),
         "preview panel keeps its width and moves by the margin");
  expect(same(widenedViewportColumns({.x0 = 0, .y0 = 0, .x1 = 255, .y1 = 239}, kNativeWidth, kWideMargin), 0, 341),
         "left half of a split frame reaches the left edge and follows the composition on the right");
  expect(same(widenedViewportColumns({.x0 = 256, .y0 = 0, .x1 = 511, .y1 = 239}, kNativeWidth, kWideMargin), 342, 683),
         "right half of a split frame follows the composition on the left and reaches the right edge");
}

} // namespace

int main() {
  clipFollowsTheGuestArithmetic();
  fourThreeIsIdentity();
  wideFrameEdgesMoveOutAndPanelsMoveWithTheComposition();
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
