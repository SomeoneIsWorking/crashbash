// crashbash_input_phase.cpp — see crashbash_input_phase.h for the key and why it is these records.

#include "crashbash_input_phase.h"

#include "core.h"
#include "crashbash_guest.h"

namespace crashbash {

std::uint64_t InputPhase::of(Core &core) const {
  return pack(core.mem_r32(guest::kSceneTransition + guest::kSceneCurrentSlot),
              core.mem_r32(guest::kMenuSceneTransition + guest::kSceneCurrentSlot));
}

} // namespace crashbash
