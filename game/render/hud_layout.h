// hud_layout.h — the guest's own HUD families, and the horizontal anchor class of each.
//
// WHERE THE CLASSES COME FROM. Every 2D element the guest draws reaches the native producer
// through one of three submit leaves, and the leaves are shared, so the leaf address cannot say
// which element this is. The element's identity here is the CALLER — `ra` at the override's entry,
// which is the guest routine that authored the rectangle. It was read out of a live run rather than
// assumed: the four tracked scenarios were replayed headless with the capture owner's per-element
// census enabled, and the census lines are reproduced below. The addresses are stable across images
// and modes because each belongs to a fixed-load image: the RESIDENT executable (`SCUS_945.70`,
// text `0x80010000+0x69000`) or BOOT (`boot_module.json`, load address `0x80078C90`).
//
//   ra          family                                                        authored x in 4:3        class
//   --------    ------------------------------------------------------------ ---------------------- ---------
//   0x80019EE8  per-player top row portraits, briefing panels, banners    x = 32 / 128 / 320 / 416 side-of-centre
//               (return from `jal 0x80029D28` at `0x80019EE0`)
//   0x8007A010  the FIRST digit of each player's lives counter             x = 43 / 139 / 331 / 427 side-of-centre
//   0x80079F9C  the SECOND digit of each player's lives counter            x = 62 / 158 / 350 / 446 side-of-centre
//   0x80079EE0  the FIRST digit of each player's 3-digit score             x = 35 / 131 / 323 / 419 side-of-centre
//   0x80079E6C  the SECOND digit of each player's 3-digit score            x = 54 / 150 / 342 / 438 side-of-centre
//   0x80079DF8  the THIRD digit of each player's 3-digit score             x = 73 / 169 / 361 / 457 side-of-centre
//               (all five digit sites are `jal 0x8002992C` inside BOOT's one HUD routine 0x800798A4;
//               the score row is what Pogo Painter and the attract demo draw under the portraits,
//               measured 2026-10-02 from the UNCLASSIFIED census of the Polar Push, Tournament Crate
//               Crush and Pogo Painter runs)
//   0x80024688  the string renderer (`jal 0x8002992C` at `0x80024680`),     centred blocks             centred
//               reached from the text walk at `0x80024400`
//   0x80024640  a flat sprite through the same walk                        see below                  centred
//   0x800245F4  the countdown / winner stack at a FIXED x                  x = 196..227                centred
//
// WHY `side-of-centre` FOR THE PER-PLAYER ROW AND NOT A CONSTANT PER ELEMENT. The four player
// panels are authored at 32, 128, 320 and 416 of 512: the outer pair sits 32 px inside a frame
// edge and the inner pair 128 px. They are one symmetric row, and the only fact that separates the
// left half from the right half is which side of the frame the panel was authored on — a property
// of the authored rectangle, read at 4:3 and therefore identical at every aspect. It is not a
// magic offset: it is the guest's own layout, and it holds for two players and four alike.
//
// WHY THE TEXT FAMILIES ARE CENTRED AND NOT CLASSIFIED PER GLYPH. A string is drawn glyph by
// glyph, and the glyphs of one centred line straddle the frame centre — a briefing line runs from
// x = 128 to x = 415, so classifying each glyph by its own rectangle would put the 'U' at the left
// edge and the 'Y' at the right one and tear the line apart. The class therefore belongs to the
// family, which is what `Family::forCaller` returns.
//
// WHY THE COUNTDOWN IS DECLARED CENTRED RATHER THAN MEASURED. Its authored rectangle is
// x = 196..227, whose centre is 211 — 45 px LEFT of the 512 frame's centre, so any rule that reads
// "left of centre means left edge" would move it 86 px further left. It is the guest's own centred
// stack (it is drawn at one fixed x and animated in y), and the class is declared rather than
// inferred from a rectangle that does not mean what its position suggests.
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
// A player's panel keeps the distance to the frame edge it was authored against: the two left-hand
// panels (portraits at x = 32 and 128, their digits at x = 43/62 and 139/158) stay against the left
// edge, the two right-hand ones (portraits at 320 and 416, digits at 331/350 and 427/446) move out
// to the right edge. The test is the authored box's CENTRE against the authored frame's centre,
// which is the same test at both aspects because both operands are 4:3 values.
//
// It applies to the per-player row ONLY. The briefing's own panels are drawn by the same caller and
// are a centred composition, and they are excluded by `isPerPlayerRow` below rather than by a
// position test that a briefing panel would fail by luck.
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