// The Crash Bash input phase: what it reads from the guest, that each half re-keys the recording,
// and that it can never produce the framework's reserved "no phase".
//
// THE NEGATIVE THAT MATTERS. psx::input::kUnkeyedPhase means "this title declares no phase"; a
// phase-keyed recording replayed under it is refused outright, and a key that produced it while the
// file claimed to be keyed would silently replay absolute-from-boot — the exact wrong-answer shape
// the phase format exists to end. So the masking is tested, not assumed: 0xFFFFFFFF in both halves
// must still not produce the reserved key.
//
// The addresses are the guest facts the drivers and the scene-machine observer already read
// (crashbash_guest.h, game/diagnostics/scene_machine.cpp): this is the cross-check that the
// recorder, the replayer and the observers all look at the same two words.

#include "core.h"
#include "crashbash_guest.h"
#include "crashbash_input_phase.h"
#include "input_phase.h"
#include "testutil.h"

#include <cstdint>
#include <memory>

namespace {

void test_addresses_are_the_guest_facts() {
  CHECK_EQ(crashbash::guest::kSceneTransition, 0x8009F658u);
  CHECK_EQ(crashbash::guest::kMenuSceneTransition, 0x8009F8A4u);
  CHECK_EQ(crashbash::guest::kSceneCurrentSlot, 0u);
}

void test_reads_the_two_scene_records() {
  const crashbash::InputPhase phase;
  // A real Core over real guest RAM: the phase is a READ of the two words, not a cached decision.
  // ON THE HEAP, and that is load-bearing: Core carries the whole PSX address space, so a `Core` on
  // the stack of a test main() runs the process off the end of the stack instead of reading RAM.
  auto core = std::make_unique<Core>();

  // Reset state: both scene records are zero (retail's scene enter clears current). A real key,
  // never the reserved one.
  const std::uint64_t reset = phase.of(*core);
  CHECK_EQ(reset, crashbash::InputPhase::pack(0, 0));
  CHECK(reset != psx::input::kUnkeyedPhase);

  // The menu world on the SELECT GAME TYPE screen: outer scene 0x8009F720, nested screen struct
  // 0x800B8E28 (the table menu_boundary.cpp observes as "current manager").
  core->mem_w32(crashbash::guest::kSceneTransition, 0x8009F720u);
  CHECK_EQ(phase.of(*core), crashbash::InputPhase::pack(0x8009F720u, 0u));
  core->mem_w32(crashbash::guest::kMenuSceneTransition, 0x800B8E28u);
  CHECK_EQ(phase.of(*core), crashbash::InputPhase::pack(0x8009F720u, 0x800B8E28u));

  // A different screen under the SAME outer scene is a different phase — a sub-screen is its own
  // segment, not an offset inside SELECT GAME TYPE.
  core->mem_w32(crashbash::guest::kMenuSceneTransition, 0x800B8E3Cu);
  CHECK(phase.of(*core) != crashbash::InputPhase::pack(0x8009F720u, 0x800B8E28u));
  CHECK_EQ(phase.of(*core), crashbash::InputPhase::pack(0x8009F720u, 0x800B8E3Cu));

  // Leaving the menu world re-keys on the OUTER half alone: a live match runs as scene 0x8009F480
  // while the menu record still holds the briefing's screen, so the match and the briefing that
  // loaded it cannot share a segment across a variable load.
  core->mem_w32(crashbash::guest::kSceneTransition, 0x8009F480u);
  CHECK_EQ(phase.of(*core), crashbash::InputPhase::pack(0x8009F480u, 0x800B8E3Cu));

  // The same words give the same key — the property the format rests on: two frames with the same
  // key are interchangeable, and nothing else about the frame may change it.
  CHECK_EQ(phase.of(*core), phase.of(*core));
}

void test_halves_do_not_alias_and_the_reserved_key_is_unreachable() {
  using crashbash::InputPhase;
  // The halves must not swap: (scene, menu) and (menu, scene) are different screens.
  CHECK(InputPhase::pack(0x8009F720u, 1u) != InputPhase::pack(1u, 0x8009F720u));
  // Masking to 24 bits drops only the constant 0x80 KSEG prefix every guest RAM address carries,
  // so no distinct pair in RAM can collide by it.
  CHECK_EQ(InputPhase::pack(0x8009F720u, 0x800B8E28u), InputPhase::pack(0x0009F720u, 0x000B8E28u));
  // And the reserved key stays unreachable even at the worst case the guest could produce.
  CHECK(InputPhase::pack(0xFFFFFFFFu, 0xFFFFFFFFu) != psx::input::kUnkeyedPhase);
}

} // namespace

int main() {
  RUN(addresses_are_the_guest_facts);
  RUN(reads_the_two_scene_records);
  RUN(halves_do_not_alias_and_the_reserved_key_is_unreachable);
  return pt_summary();
}
