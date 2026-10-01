// ui_anchor.h — THE ONE horizontal anchoring policy for Crash Bash's authored screen-space UI.
//
// WHY THIS EXISTS. Widescreen widens the canvas from the guest's 512 columns to 684. Everything that
// is a PROJECTION moves by itself, because the projection's horizontal centre moved. Nothing that
// is a FIXED 4:3 layout moves by itself, because the guest authored it against 512 — so the
// framework's queue answers every 2D producer with one rule, "authored 4:3 x is CENTRED in the wide
// frame" (`RQ_2D_AUTHORED_4_3`, psxport `render_queue.h` / `rq_2d_xform`), and centring is the
// right answer for exactly one class of element. It is the wrong answer for the rest, and it is
// wrong in a way no capture of the world can show:
//
//   * the four per-player panels are the CORNER panels. Their portraits are authored at x = 32,
//     128, 320 and 416 of 512 — 32 px inside the left edge and 32 px inside the right one. Centred,
//     they slide 86 px INTO the arena the widening just revealed and stop being corner panels;
//   * a right-hand element has to move by the FULL widening (172 px), not by half, or its distance
//     from the right edge doubles.
//
// The second reason this is one owner and not a per-producer constant: the arithmetic is three
// lines, and three lines copied into each producer is several places to be wrong and no place that
// can be tested. The CLASS is a fact about the ELEMENT, so it is declared where the element is
// authored — `hud_layout.h` — and the arithmetic is here.
//
// IT IS A PRESENTATION CHANGE AND NOTHING ELSE. `place` takes an authored x and returns a drawn x;
// it neither reads nor writes guest memory, and it is not allowed to. The HUD quads this places are
// the PORT's own decoded draw records (`SpriteQuadDraw`) on the way to the rasterizer, and the
// guest's own Moby/sprite records are untouched. At 4:3 every entry point here is the identity, so
// the 4:3 picture is unchanged by construction rather than by a saved copy.
#pragma once

#include <cstdint>
#include <optional>

struct Core;

namespace crashbash::render::ui_anchor {

// The class of element. It is a property of the LAYOUT and never of the current aspect: a panel
// that is edge-anchored at 4:3 is edge-anchored at 16:9. Asking the aspect is what produces a HUD
// that migrates across the screen when the setting changes.
enum class Anchor : std::uint8_t {
  // Authored distance to the LEFT edge is preserved: the drawn x equals the authored x, so the
  // element keeps the same inset from the widened frame's left edge.
  LeftEdge = 0,
  // The authored box keeps its distance to the frame's CENTRE — the framework's existing
  // `RQ_2D_AUTHORED_4_3` rule, and the right answer for a title, a countdown, a banner or a text
  // block the guest laid out around a centre.
  Centred = 1,
  // Authored distance to the RIGHT edge is preserved: the element moves by the FULL widening, twice
  // what a centred one does.
  RightEdge = 2,
};

inline const char *name(Anchor anchor) {
  switch (anchor) {
  case Anchor::LeftEdge:
    return "left-edge";
  case Anchor::Centred:
    return "centred";
  case Anchor::RightEdge:
    return "right-edge";
  }
  return "unknown";
}

// The two frame widths this policy relates: the width the guest authored its layout against, and
// the width this port presents into. They are equal at 4:3, which is what makes every answer below
// the identity there.
struct Frame {
  std::int32_t authored = 0;
  std::int32_t drawn = 0;
};

// Why an authored box was refused. Every refusal is a value the PORT got wrong, and a refused
// element is left exactly where the guest put it rather than moved by a number derived from it.
enum class Refusal : std::uint8_t {
  None = 0,
  NonPositiveWidth,
  NarrowerFrame,
  OutsideAuthoredFrame,
  UnknownClass,
};

inline const char *refusalName(Refusal refusal) {
  switch (refusal) {
  case Refusal::None:
    return "none";
  case Refusal::NonPositiveWidth:
    return "non-positive-width";
  case Refusal::NarrowerFrame:
    return "narrower-frame";
  case Refusal::OutsideAuthoredFrame:
    return "outside-authored-frame";
  case Refusal::UnknownClass:
    return "unknown-class";
  }
  return "unknown";
}

// A placed box, in the DRAWN frame. `x` is inclusive-left like every guest rectangle in this port,
// and `width` is the AUTHORED width in every class: nothing is stretched, so a digit is the same
// number of pixels wide at 16:9 as at 4:3 and only its origin moves.
struct Box {
  std::int32_t x = 0;
  std::int32_t width = 0;
};

struct Placed {
  Box box{};
  // The horizontal displacement applied to the authored x. Zero at 4:3 for every class, which is
  // the property the 4:3 identity rests on.
  std::int32_t offset = 0;
  Refusal refusal = Refusal::None;
};

constexpr bool known(Anchor anchor) {
  return anchor == Anchor::LeftEdge || anchor == Anchor::Centred || anchor == Anchor::RightEdge;
}

// The frame is checked before anything is placed, because a "widened" frame that is not wider is a
// misread width and every offset derived from it would be silently wrong.
constexpr Refusal checkFrame(const Frame &frame) {
  if (frame.authored <= 0 || frame.drawn <= 0) {
    return Refusal::NonPositiveWidth;
  }
  if (frame.drawn < frame.authored) {
    return Refusal::NarrowerFrame;
  }
  return Refusal::None;
}

// Why an element of this class cannot be anchored into this frame, or None.
constexpr Refusal refusalFor(Anchor anchor, const Frame &frame) {
  return known(anchor) ? checkFrame(frame) : Refusal::UnknownClass;
}

// The one horizontal margin: how much wider the drawn frame is than the authored one, halved. At
// 4:3 it is zero, and it is the unit the whole policy is written in — a centred element moves by
// one margin, a right-edge element by two.
constexpr std::int32_t margin(const Frame &frame) {
  return (frame.drawn - frame.authored) / 2;
}

// How far an anchored element's ORIGIN moves, in the drawn frame. `Centred` moves by the margin,
// `LeftEdge` not at all, `RightEdge` by the whole widening.
constexpr std::optional<std::int32_t> offset(Anchor anchor, const Frame &frame) {
  if (refusalFor(anchor, frame) != Refusal::None) {
    return std::nullopt;
  }
  switch (anchor) {
  case Anchor::LeftEdge:
    return 0;
  case Anchor::Centred:
    return margin(frame);
  case Anchor::RightEdge:
    return frame.drawn - frame.authored;
  }
  return std::nullopt;
}

// The correction to apply to an x the FRAMEWORK has ALREADY shifted by its centring rule.
//
// Every sprite quad this title submits goes through psxport's 2D layout transform, which shifts an
// authored 4:3 x by the margin before it reaches the rasterizer. So an anchored element needs the
// DIFFERENCE between its own class and the class that transform already applied — not its own
// offset, or it would move twice. At 4:3 both are zero and the correction is the identity; at 16:9
// a left-edge element gets `-margin` (back to its authored inset from the new left edge), a
// centred one gets 0 (it is already centred, which is correct), and a right-edge one gets `+margin`.
//
// It is a correction and not a replacement precisely so the two halves of the same HUD cannot
// disagree: this owner never has to fight the framework's rule, it only states the difference.
constexpr std::optional<std::int32_t> correction(Anchor anchor, const Frame &frame) {
  const std::optional<std::int32_t> own = offset(anchor, frame);
  if (!own) {
    return std::nullopt;
  }
  return *own - margin(frame);
}

// The placed box for an authored element. `authoredX`/`authoredWidth` are the guest's own values;
// the returned x is in the drawn frame and the returned width is the authored one, because an
// anchored element is MOVED, never resized.
constexpr Placed place(Anchor anchor, std::int32_t authoredX, std::int32_t authoredWidth, const Frame &frame) {
  Placed placed;
  if (const Refusal refusal = refusalFor(anchor, frame); refusal != Refusal::None) {
    placed.refusal = refusal;
    return placed;
  }
  if (authoredWidth <= 0) {
    placed.refusal = Refusal::NonPositiveWidth;
    return placed;
  }
  if (authoredX < 0 || authoredX + authoredWidth > frame.authored) {
    placed.refusal = Refusal::OutsideAuthoredFrame;
    return placed;
  }
  const std::int32_t shift = *offset(anchor, frame);
  placed.box = {authoredX + shift, authoredWidth};
  placed.offset = shift;
  return placed;
}

// ── The Core-facing half ────────────────────────────────────────────────────────────────────────
//
// Everything above is pure arithmetic and is what the focused test exercises. These are the only
// things that touch the running product, and they exist so a producer cannot report a different
// number from the one it drew.

// The frame this policy relates: the guest's own 4:3 width and the width this port presents into,
// read from the ONE owner that knows both (`gpu_vk_native_w` / `gpu_vk_wide_engine_w`).
Frame frame(Core *core);

// The correction this element needs, AND the report of it, in one call: a producer cannot draw a
// different shift from the one the log read, because there is only one number and it is produced
// once. A refused element is corrected by zero and reported with its reason. `logicFrame` is the
// guest's own frame counter, so a report names the frame the number was drawn on.
std::int32_t correctionAndReport(std::uint32_t logicFrame,
                                 const char *element,
                                 Anchor anchor,
                                 std::int32_t authoredX,
                                 std::int32_t authoredWidth,
                                 const Frame &frame);

} // namespace crashbash::render::ui_anchor