#pragma once

#include <cstdint>

class Core;

namespace crashbash {

// The INPUT PHASE a Crash Bash pad recording is keyed on: the guest's own scene record, packed so
// each recorded press is an offset from the entry of the SCREEN it was recorded on. Frame counts are
// deliberately not the key — boot and load timing move a frame count without moving the screen.
//
// Both halves are guest-owned CURRENT slots (crashbash_guest.h): the outer record (kSceneTransition)
// separates boot, the loading handoff, the menu world and the LOAD GAME dialog; the menu record
// (kMenuSceneTransition) separates the menu's own sub-screens. Both are stable while their screen is,
// and neither is the selection cursor, which moves on every d-pad press.
//
// The Polar Push briefing and its running match measure the same two words and therefore share one
// segment; that is sound for a recording, and it means no recording can be cut at the match start
// from these two words alone.
//
// Each half keeps only its low 24 bits, which drops the constant 0x80 KSEG prefix every guest RAM
// address carries. That makes psx::input::kUnkeyedPhase — both halves all-ones — structurally
// unreachable: a title that produced it would have its keyed recording replayed as absolute from
// boot.
//
// Stateless by construction — every answer is read from the Core — and held by value in the runtime
// so no process-global state can select a different instance.
class InputPhase {
public:
  // The phase for this pad frame: the two scene-record CURRENT slots, outer scene in the high half
  // and menu screen in the low half. A title declares no phase at all by answering
  // psx::input::kUnkeyedPhase (GameRuntime's default); this one never does, by the masking above.
  std::uint64_t of(Core &core) const;

  // The packed form, exposed so the test cross-checks it against the header's addresses without
  // restating the packing.
  static constexpr std::uint64_t pack(std::uint32_t scene, std::uint32_t menuScene) {
    return (static_cast<std::uint64_t>(scene & 0xFFFFFFu) << 24) | (menuScene & 0xFFFFFFu);
  }
};

} // namespace crashbash
