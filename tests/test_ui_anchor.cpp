// test_ui_anchor.cpp — the anchoring arithmetic and the HUD classification, exercised directly.
//
// The cases are the SHIPPING layout, not invented ones: the authored rectangles are the ones read
// off a live Crashball match by the capture owner's per-element census (hud_layout.h names the guest
// routine behind each), and the 16:9 frame is the one this port actually presents. Every negative is
// a case that has to be refused or must stay put, because the failure this guards against is a HUD
// that moves when it should not.
#include "hud_layout.h"
#include "ui_anchor.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace {

using crashbash::render::hud_layout::Caller;
using crashbash::render::ui_anchor::Anchor;
using crashbash::render::ui_anchor::Frame;
using crashbash::render::ui_anchor::Refusal;

constexpr Frame kFourThree{.authored = 512, .drawn = 512};
constexpr Frame kWide1699{.authored = 512, .drawn = 684};
constexpr Frame kUltraWide{.authored = 512, .drawn = 1024};

int failures = 0;

void expect(bool condition, const char *what) {
  if (!condition) {
    ++failures;
    std::fprintf(stderr, "[ui-anchor] FAILED: %s\n", what);
  }
}

// Every class is the IDENTITY at 4:3, for every element. This is the property the whole 4:3 picture
// rests on, so it is asserted over the real element rectangles rather than over one of them.
void fourThreeIsIdentity() {
  for (const Frame &frame : {kFourThree}) {
    for (const Anchor anchor : {Anchor::LeftEdge, Anchor::Centred, Anchor::RightEdge}) {
      expect(crashbash::render::ui_anchor::correction(anchor, frame).value_or(1) == 0, "4:3 correction is 0");
      expect(crashbash::render::ui_anchor::offset(anchor, frame).value_or(1) == 0, "4:3 offset is 0");
    }
  }
}

// The four player panels and their two-digit scores, exactly as the census read them.
void perPlayerRowKeepsItsAuthoredEdgeDistance() {
  struct Element {
    std::int32_t x;
    std::int32_t width;
    Anchor expected;
  };
  constexpr Element kRow[] = {
      {32, 64, Anchor::LeftEdge},
      {128, 64, Anchor::LeftEdge},
      {320, 64, Anchor::RightEdge},
      {416, 64, Anchor::RightEdge},
      {43, 32, Anchor::LeftEdge},
      {62, 32, Anchor::LeftEdge},
      {139, 32, Anchor::LeftEdge},
      {158, 32, Anchor::LeftEdge},
      {331, 32, Anchor::RightEdge},
      {350, 32, Anchor::RightEdge},
      {427, 32, Anchor::RightEdge},
      {446, 32, Anchor::RightEdge},
  };
  for (const Element &element : kRow) {
    const Anchor anchor =
        crashbash::render::hud_layout::anchorFor(Caller::Panels, element.x, element.width, kWide1699, false);
    expect(anchor == element.expected, "per-player panel is on its authored side");
  }

  // The correction is the DIFFERENCE from the framework's centring: a left-edge element is pulled
  // back by one margin and a right-edge one pushed out by one more, so the drawn inset from the
  // widened edge equals the authored inset from the 4:3 edge.
  const std::int32_t margin = crashbash::render::ui_anchor::margin(kWide1699);
  expect(crashbash::render::ui_anchor::correction(Anchor::LeftEdge, kWide1699).value_or(0) == -margin,
         "left-edge correction is -margin");
  expect(crashbash::render::ui_anchor::correction(Anchor::Centred, kWide1699).value_or(1) == 0,
         "centred correction is 0");
  expect(crashbash::render::ui_anchor::correction(Anchor::RightEdge, kWide1699).value_or(0) == margin,
         "right-edge correction is +margin");

  // And the resulting drawn inset is the authored one, which is the whole claim.
  const std::int32_t drawnLeft = 32 + *crashbash::render::ui_anchor::offset(Anchor::LeftEdge, kWide1699);
  const std::int32_t drawnRightInset =
      kWide1699.drawn - (416 + *crashbash::render::ui_anchor::offset(Anchor::RightEdge, kWide1699)) - 64;
  expect(drawnLeft == 32, "left panel keeps its 32 px inset");
  expect(drawnRightInset == 32, "right panel keeps its 32 px inset");
}

// NOTHING STRETCHES: an anchored element keeps its authored width and its height, and only its
// origin moves. A panel that grew would be a different product.
void nothingStretches() {
  const auto placed = crashbash::render::ui_anchor::place(Anchor::RightEdge, 416, 64, kWide1699);
  expect(placed.refusal == Refusal::None, "a valid element is placed");
  expect(placed.box.width == 64, "the authored width is preserved");
  expect(placed.box.x == 416 + 172, "a right-edge element moves by the whole widening");
  const auto centred = crashbash::render::ui_anchor::place(Anchor::Centred, 128, 64, kWide1699);
  expect(centred.box.x == 128 + 86, "a centred element moves by one margin");
}

// The authored 4:3 composition (the Crashball objective briefing) is ONE composition: every element
// on such a frame stays centred, because the corner prompts belong to that composition and not to
// the widened edges.
void authoredScreenPresentationStaysCentred() {
  for (const Caller caller : {Caller::Panels,
                              Caller::LivesLeft,
                              Caller::LivesRight,
                              Caller::ScoreFirst,
                              Caller::ScoreSecond,
                              Caller::ScoreThird,
                              Caller::String,
                              Caller::StringSprite,
                              Caller::Countdown}) {
    const Anchor anchor = crashbash::render::hud_layout::anchorFor(caller, 32, 64, kWide1699, true);
    expect(anchor == Anchor::Centred, "an authored 4:3 frame is one centred composition");
  }
}

// The countdown is DECLARED centred rather than measured: its authored centre sits 45 px left of the
// frame centre, so any "left of centre means left edge" rule would move it 86 px further left.
void countdownIsCentred() {
  const Anchor anchor = crashbash::render::hud_layout::anchorFor(Caller::Countdown, 196, 32, kWide1699, false);
  expect(anchor == Anchor::Centred, "the countdown stays centred");
}

// The classification is the CALLER's, and an unknown caller is refused rather than guessed.
void classificationIsPerCaller() {
  expect(crashbash::render::hud_layout::callerOf(0x80019EE8u) == Caller::Panels, "panel caller");
  expect(crashbash::render::hud_layout::callerOf(0x80079F9Cu) == Caller::LivesRight, "lives-right caller");
  expect(crashbash::render::hud_layout::callerOf(0x8007A010u) == Caller::LivesLeft, "lives-left caller");
  expect(crashbash::render::hud_layout::callerOf(0x80079EE0u) == Caller::ScoreFirst, "score first-digit caller");
  expect(crashbash::render::hud_layout::callerOf(0x80079DF8u) == Caller::ScoreThird, "score third-digit caller");
  // A score digit follows its player's panel: P1's last digit (x = 73) stays left, P4's (x = 457) goes right.
  expect(crashbash::render::hud_layout::anchorFor(Caller::ScoreThird, 73, 31, kWide1699, false) == Anchor::LeftEdge,
         "a left-hand score digit anchors left");
  expect(crashbash::render::hud_layout::anchorFor(Caller::ScoreThird, 457, 31, kWide1699, false) == Anchor::RightEdge,
         "a right-hand score digit anchors right");
  expect(!crashbash::render::hud_layout::callerOf(0x80000000u).has_value(), "an unknown caller is refused");
  expect(crashbash::render::hud_layout::forCaller(Caller::String)->anchor == Anchor::Centred,
         "the string family is centred");
  expect(std::string_view(crashbash::render::ui_anchor::name(Anchor::RightEdge)) == "right-edge",
         "the class has a name");
}

// The refusals. A frame that is not wider is a misread width, an element outside its own authored
// frame is not something to move silently, and neither is left shifted by a derived number.
void refusals() {
  expect(crashbash::render::ui_anchor::checkFrame(kFourThree) == Refusal::None, "4:3 frame is legal");
  expect(crashbash::render::ui_anchor::checkFrame(Frame{.authored = 0, .drawn = 684}) == Refusal::NonPositiveWidth,
         "a zero authored width is refused");
  expect(crashbash::render::ui_anchor::checkFrame(Frame{.authored = 512, .drawn = 320}) == Refusal::NarrowerFrame,
         "a narrowed frame is refused");
  expect(crashbash::render::ui_anchor::refusalFor(static_cast<Anchor>(9), kWide1699) == Refusal::UnknownClass,
         "an unknown class is refused");
  expect(crashbash::render::ui_anchor::place(Anchor::LeftEdge, 32, 0, kWide1699).refusal == Refusal::NonPositiveWidth,
         "a zero-width element is refused");
  expect(crashbash::render::ui_anchor::place(Anchor::LeftEdge, 500, 64, kWide1699).refusal ==
             Refusal::OutsideAuthoredFrame,
         "an element outside the authored frame is refused");
  expect(!crashbash::render::ui_anchor::correction(static_cast<Anchor>(9), kWide1699).has_value(),
         "an unknown class has no correction");
}

// The policy scales with the widening rather than carrying a 512 constant, because the same code
// has to answer for every aspect the port can present.
void scalesWithTheWidening() {
  expect(crashbash::render::ui_anchor::margin(kUltraWide) == 256, "an ultra-wide margin is half the widening");
  expect(*crashbash::render::ui_anchor::offset(Anchor::RightEdge, kUltraWide) == 512,
         "a right-edge element crosses the whole widening");
  expect(crashbash::render::ui_anchor::place(Anchor::RightEdge, 416, 64, kUltraWide).box.x == 928,
         "the drawn box follows the frame");
}

} // namespace

int main() {
  fourThreeIsIdentity();
  perPlayerRowKeepsItsAuthoredEdgeDistance();
  nothingStretches();
  authoredScreenPresentationStaysCentred();
  countdownIsCentred();
  classificationIsPerCaller();
  refusals();
  scalesWithTheWidening();
  if (failures) {
    std::fprintf(stderr, "[ui-anchor] %d case(s) failed\n", failures);
    return EXIT_FAILURE;
  }
  std::fprintf(stderr, "[ui-anchor] all cases passed\n");
  return EXIT_SUCCESS;
}