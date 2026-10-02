#pragma once

#include <cstdint>

namespace crashbash::render {

// Crash Bash draws a frame as one or more VIEWPORTS. Resident 0x80018B08 begins each one from a guest
// record: it links a DR_ENV whose clip is the viewport rectangle into the viewport's own slice of the
// ordering table, points the insertion base 0x800569D8 at that slice, publishes the record at
// 0x800569A8, and installs the projection centre. Every model and screen quad the guest then submits is
// clipped to that rectangle and ordered within that slice. The CHOOSE LEVEL / TOURNAMENT menus use this
// for their arena previews: five small viewports inside panels, each with its own clip and slice.
//
// The record's rectangle is authored in 640x480 units, like every other Crash Bash screen coordinate.
struct ViewportRecord {
  std::int16_t left = 0;
  std::int16_t top = 0;
  std::int16_t right = 0;
  std::int16_t bottom = 0;
};

// The viewport's inclusive clip rectangle in framebuffer pixels, relative to the framebuffer origin the
// guest's DR_ENV offset names (the producer adds the presented draw offset).
struct ViewportClip {
  std::int32_t x0 = 0;
  std::int32_t y0 = 0;
  std::int32_t x1 = -1;
  std::int32_t y1 = -1;
};

struct ViewportColumns {
  std::int32_t x0 = 0;
  std::int32_t x1 = -1;
};

// 0x80018B08's own arithmetic: x = left * displayScale / 640, y = top / 2, width = (right - left) *
// displayScale / 640, height = (bottom - top) / 2, every division truncating toward zero as the guest's
// does, then SetDefDrawEnv's clip spans [x, x + width - 1] x [y, y + height - 1].
ViewportClip viewportClip(const ViewportRecord &record, std::int16_t displayScale);

// The clip's columns in the widened frame. Widening adds `margin` columns on each side of the native
// `nativeWidth` frame and moves every projection origin by `margin`. A clip edge ON the native frame's
// edge is the screen's edge, so it moves out to the widened edge and the margins show what the view would;
// an interior edge bounds a panel inside the centred 4:3 composition and moves with it by `margin`.
// At 4:3 the margin is zero and this is the identity.
ViewportColumns widenedViewportColumns(const ViewportClip &clip, std::int32_t nativeWidth, std::int32_t margin);

} // namespace crashbash::render
