// crashbash_input_phase.h — the input phase a Crash Bash pad recording is keyed on.
//
// WHY A PHASE. A .pad file used to be one mask per pad frame counted from boot, so anything that
// moves the frame count — a slower CD read, one more boot frame, a different enhancement
// configuration — shifts every press after it, and the recording then answers a screen it was
// never recorded against. Keying each frame on WHICH SCREEN is taking input stores presses as an
// offset from that screen's entry; boot and load timing move the phase boundary instead of the
// presses inside it. The framework compares phases and records when they change; it does not know
// what a phase means, so the vocabulary is this file's. See psxport's input_phase.h.
//
// THE KEY, both halves the guest's own scene records (crashbash_guest.h):
//
//   * the OUTER record (kSceneTransition, 0x8009F658) is the root scene machine, and its CURRENT is
//     what separates boot (0x800B9524), every loading handoff (0x800A00DC), the menu world
//     (0x8009F720) and the LOAD GAME dialog (0x8009F480 — measured 2026-10-02: its outer scene while
//     the card warning "THERE IS NO CRASH BASH DATA ON THIS MEMORY CARD" is up).
//   * the MENU record (kMenuSceneTransition, 0x8009F8A4) is the menu world's nested scene machine,
//     whose current is the active menu screen's scene struct — 0x800B8E28 for SELECT GAME TYPE,
//     0x800B8E3C, 0x800B9DF4 character select, 0x800BA72C CHOOSE LEVEL, 0x800BAAB4 VS BATTLE,
//     0x800B8E8C options — which is what distinguishes the sub-screens that all run under the one
//     outer scene.
//
// Both currents are stable while their screen is: each changes only when the retail scene machine
// completes a transition. Deliberately NOT the selection cursor (0x800B95F0): it moves on every
// d-pad press, so a key built from it would re-key on the press itself and every recorded input
// would land at offset 0 of a brand-new segment.
//
// WHAT THE KEY DOES NOT SEPARATE. The Polar Push briefing and the running Polar Push match both
// measure outer 0x8009F720 with menu 0x00000000, so they share one segment. That is sound for a
// recording — a press is replayed at its offset from the segment's entry either way — and it is the
// reason no recording can be cut "at the moment the match starts" from these two words alone; the
// measured boundary is the P1 record word 0x800AD304 answering a held direction.
//
// MASKING. Each half keeps only its low 24 bits, which drops the constant 0x80 KSEG prefix every
// guest RAM address carries (PSX RAM is 2 MB, so the low 24 bits already distinguish every address
// in it). That makes psx::input::kUnkeyedPhase — both halves all-ones — structurally unreachable:
// a title that produced it would have its keyed recording silently replayed as absolute from boot.
#pragma once

#include <cstdint>

class Core;

namespace crashbash {

// Stateless by construction — every answer is read from the Core — and held by value in the
// runtime so no process-global state can select a different instance.
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
