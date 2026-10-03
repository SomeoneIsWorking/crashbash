// hud_layout.h — the guest's own HUD families, and the horizontal anchor class of each.
//
// Every 2D element the guest draws reaches the native producer through one of three shared submit
// leaves, so the leaf address cannot say which element this is. The element's identity here is the
// CALLER — `ra` at the override's entry, the guest routine that authored the rectangle — and the
// addresses below are stable across images and modes because each belongs to a fixed-load image
// (the RESIDENT executable, or BOOT at `0x80078C90`).
//
//   ra          family                                                        authored x in 4:3        class
//   --------    ------------------------------------------------------------ ---------------------- ---------
//   0x80019EE8  per-player top row portraits, briefing panels, banners    x = 32 / 128 / 320 / 416 side-of-centre
//   0x8007A010  the FIRST digit of each player's lives counter             x = 43 / 139 / 331 / 427 side-of-centre
//   0x80079F9C  the SECOND digit of each player's lives counter            x = 62 / 158 / 350 / 446 side-of-centre
//   0x80079EE0  the FIRST digit of each player's 3-digit score             x = 35 / 131 / 323 / 419 side-of-centre
//   0x80079E6C  the SECOND digit of each player's 3-digit score            x = 54 / 150 / 342 / 438 side-of-centre
//   0x80079DF8  the THIRD digit of each player's 3-digit score             x = 73 / 169 / 361 / 457 side-of-centre
//   0x80024688  the string renderer, reached from the text walk at 0x80024400 centred blocks   centred
//   0x80024640  a flat sprite through the same walk                                            centred
//   0x800245F4  the countdown / winner stack at a FIXED x                  x = 196..227                centred
//
// The per-player row is classified side-of-centre rather than by a constant per element: the row is
// symmetric, so the only fact separating its two halves is which side of the frame a panel was
// authored on, which is a 4:3 property and identical at every aspect.
//
// The text families are centred as a FAMILY, not per glyph: the glyphs of one centred line straddle
// the frame centre, so classifying each glyph by its own rectangle would tear a line apart. The
// countdown is declared centred rather than inferred: its authored centre (211) is 45 px left of the
// 512 frame's centre, so a "left of centre means left edge" rule would move it further left.
#pragma once

#include "ui_anchor.h"

#include <cstdint>
#include <optional>

namespace crashbash::render::hud_layout {

// The guest routine that authored a HUD element, by the return address its submit leaf saw.
enum class Caller : std::uint32_t {
  // RESIDENT: the panel submitter, `jal 0x80029D28` at `0x80019EE0`.
  Panels = 0x80019EE8u,
  // RESIDENT: the string renderer's per-glyph submit, `jal 0x8002992C` at `0x80024680`.
  String = 0x80024688u,
  // RESIDENT: the same walk's flat-sprite submit, `jal 0x80029D28` at `0x80024638`.
  StringSprite = 0x80024640u,
  // RESIDENT: the fixed-x countdown / winner stack, `jal 0x80029D28` at `0x800245EC`.
  Countdown = 0x800245F4u,
  // BOOT: the per-player lives digits. One routine submits each player's first digit
  // (x = 43/139/331/427) and the other each player's second (x = 62/158/350/446), which is what
  // pairs them into the 2-digit counters read off a live Crashball match. Neither names a screen
  // side: all four players use both, and the side is a fact about the authored rectangle.
  LivesRight = 0x80079F9Cu,
  LivesLeft = 0x8007A010u,
  // BOOT: the per-player 3-digit score, one submit site per digit in the same HUD routine
  // (0x800798A4) as the lives digits. Each site serves all four players; the side is again a fact
  // about the authored rectangle.
  ScoreFirst = 0x80079EE0u,
  ScoreSecond = 0x80079E6Cu,
  ScoreThird = 0x80079DF8u,
};

// What a family is, for anchoring and for the report that names it.
struct Family {
  const char *name;
  ui_anchor::Anchor anchor;
};

// The family a caller belongs to, or nullopt for a caller this port has not classified. An
// unclassified family is not guessed at: the producer leaves such a quad on the framework's own
// centring rule and says so, because an unknown element moved by an assumed class is the class of
// quiet wrong answer this project keeps finding.
constexpr std::optional<Family> forCaller(Caller caller) {
  switch (caller) {
  case Caller::Panels:
    // The panel family mixes the corner portraits with the briefing's own panels, and the two are
    // separated by the frame they are drawn in rather than by the caller — see `anchorsTo`.
    return Family{"panel", ui_anchor::Anchor::Centred};
  case Caller::String:
  case Caller::StringSprite:
    return Family{"string", ui_anchor::Anchor::Centred};
  case Caller::Countdown:
    return Family{"countdown", ui_anchor::Anchor::Centred};
  case Caller::LivesRight:
  case Caller::LivesLeft:
    return Family{"lives", ui_anchor::Anchor::Centred};
  case Caller::ScoreFirst:
  case Caller::ScoreSecond:
  case Caller::ScoreThird:
    return Family{"score", ui_anchor::Anchor::Centred};
  }
  return std::nullopt;
}

constexpr std::optional<Caller> callerOf(std::uint32_t returnAddress) {
  switch (returnAddress) {
  case static_cast<std::uint32_t>(Caller::Panels):
    return Caller::Panels;
  case static_cast<std::uint32_t>(Caller::String):
    return Caller::String;
  case static_cast<std::uint32_t>(Caller::StringSprite):
    return Caller::StringSprite;
  case static_cast<std::uint32_t>(Caller::Countdown):
    return Caller::Countdown;
  case static_cast<std::uint32_t>(Caller::LivesRight):
    return Caller::LivesRight;
  case static_cast<std::uint32_t>(Caller::LivesLeft):
    return Caller::LivesLeft;
  case static_cast<std::uint32_t>(Caller::ScoreFirst):
    return Caller::ScoreFirst;
  case static_cast<std::uint32_t>(Caller::ScoreSecond):
    return Caller::ScoreSecond;
  case static_cast<std::uint32_t>(Caller::ScoreThird):
    return Caller::ScoreThird;
  }
  return std::nullopt;
}

// THE PER-PLAYER ROW, and the one place a class is read from a rectangle instead of declared.
//
// A player's panel keeps the distance to the frame edge it was authored against, so the test is the
// authored box's CENTRE against the authored frame's centre — the same test at every aspect, because
// both operands are 4:3 values. The briefing's own panels are drawn by the same caller and are a
// centred composition, so membership of the row (not a position test) is what selects this rule.
constexpr bool isPerPlayerRow(Caller caller) {
  return caller == Caller::Panels || caller == Caller::LivesLeft || caller == Caller::LivesRight ||
         caller == Caller::ScoreFirst || caller == Caller::ScoreSecond || caller == Caller::ScoreThird;
}

// The class of one per-player element, from the rectangle the guest authored for it.
constexpr ui_anchor::Anchor
perPlayerAnchor(std::int32_t authoredX, std::int32_t authoredWidth, const ui_anchor::Frame &frame) {
  const std::int32_t centre = authoredX + authoredWidth / 2;
  return centre * 2 < frame.authored ? ui_anchor::Anchor::LeftEdge : ui_anchor::Anchor::RightEdge;
}

// The class of an element, given the caller that authored it and the rectangle it was authored at.
// `onAuthoredScreenPresentation` suppresses every per-player test on a frame the guest authored as
// ONE 4:3 composition (the Crashball objective briefing): there the whole frame is a single
// centred composition, and its corner prompts belong to that composition, not to the widened edges.
constexpr ui_anchor::Anchor anchorFor(Caller caller,
                                      std::int32_t authoredX,
                                      std::int32_t authoredWidth,
                                      const ui_anchor::Frame &frame,
                                      bool onAuthoredScreenPresentation) {
  if (onAuthoredScreenPresentation || !isPerPlayerRow(caller)) {
    return ui_anchor::Anchor::Centred;
  }
  return perPlayerAnchor(authoredX, authoredWidth, frame);
}

} // namespace crashbash::render::hud_layout