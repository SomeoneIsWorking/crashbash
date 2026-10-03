#pragma once

#include <cstdint>
#include <cstdio>

class Core;
class Pad;

namespace crashbash {

// A title-owned control-channel request to enter an arena, and the owner that applies it.
//
// Modelled on Tomba! 2's `dev_warp`: the control channel only ARMS a request, and the frame driver
// applies it at a frame boundary by running the game's OWN menu flow — never by writing a phase, a
// scene pointer or a timer into guest RAM.
//
// The flow is the one a player walks: SELECT GAME TYPE's accept installs the mode's screen-flow table
// through `FUN_800b5360`, the menu machine walks that flow to SELECT BATTLE TYPE, and that screen's
// own update (`FUN_800b7458`) reads the cursor pair `DAT_8005a64a` / `DAT_8005a64b` to index the
// guest-owned per-page arena table at `0x800BA324` and writes the selected entry's arena into
// `0x8009E5DC`. The host selects the entry by setting the cursor the screen itself reads and then
// lets the guest do every step; nothing here writes the arena, a phase or a scene.
//
//   arena list         — the guest's own arena table, read from its memory
//   arena <id> [mode]  — arm a request; `battle` (default) or `tournament`
//
// This exists only on the control channel. A player's route never sees it.
class DevArena {
public:
  // Returns true when `cmd` was this title's command and the reply has been written to `out`.
  static bool handle(Core &core, const char *cmd, const char *line, std::FILE *out);

  // Called by the frame driver at every frame boundary. Advances an armed request one step. `pad`
  // is the title's own controller: the attract demo is left with a real Start press through it,
  // never by writing guest input state.
  static void applyArmed(Core &core, Pad &pad, std::uint32_t frame);

  static void reset();

private:
  enum class ArenaMode : std::uint32_t {
    Battle = 0,
    Tournament = 1,
  };

  enum class Stage : std::uint32_t {
    // The attract demo owns the nested overlay slot, so the menu's own table is not there yet. The
    // game's own way out of the demo is a Start press, delivered through the pad.
    EnterMenu = 0,
    // The mode's flow table has not been requested yet.
    SelectMode = 1,
    // The flow is running; waiting for SELECT BATTLE TYPE to become the current menu screen.
    AwaitArenaScreen = 2,
    // SELECT BATTLE TYPE is up and the cursor names the requested entry; the screen's own update
    // writes the arena. Waiting for the guest's arena selection to equal the request.
    AwaitCommit = 3,
  };

  struct Request {
    bool armed = false;
    std::uint32_t arena = 0;
    ArenaMode mode = ArenaMode::Battle;
    // The entry's page and index, located in the guest's own table when the request is armed.
    std::uint32_t page = 0;
    std::uint32_t index = 0;
    Stage stage = Stage::EnterMenu;
    std::uint32_t firstFrame = 0;
    // Set once this request has driven Start itself. The pad reports `repl_on` for its own forced
    // drive as well as for a replay, so ownership is only decidable before the first tap.
    bool tapped = false;
    // The screen the request last saw, so a walk through the menu flow is reported as it happens.
    std::uint32_t lastScreen = 0;
  };

  // Bounded so a request that never reaches its screen cannot hold the frame driver forever.
  static inline constexpr std::uint32_t kFrameBudget = 2400u;
  // How often a request that is still waiting reports which module owns the menu slot, and how often
  // it re-presses Start to leave the attract demo.
  static inline constexpr std::uint32_t kWaitReportStride = 120u;
  // Frames between Start presses while the menu has not come up.
  static inline constexpr std::uint32_t kStartRetryStride = 90u;
  // Start and Cross are active-low on the pad's own mask, exactly as the control channel's
  // `tap start` / `tap x` drive them.
  static inline constexpr std::uint16_t kStartPressed = 0xFFFF & ~0x0008u;
  static inline constexpr std::uint16_t kConfirmPressed = 0xFFFF & ~0x4000u;
  static inline constexpr int kPressFrames = 4;
  // Frames between confirm presses while the menu flow walks its screens. Every screen of the
  // battle flow except SELECT BATTLE TYPE is advanced by the game's own accept, which is a Cross
  // press on the pad — the same press a player makes, through the same input path.
  static inline constexpr std::uint32_t kFlowPressStride = 30u;
  static Request request_;
};

} // namespace crashbash