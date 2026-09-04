#pragma once

#include "core.h"
#include "guest_execution.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace crashbash {

// One owner for the guest-call mechanics used by the RE-derived boot and frame modules. The return
// address and instruction count at each call site come from the emitted retail body.
template <typename... Args>
std::uint32_t measuredGuestCall(
    Core &core, std::uint32_t target, std::uint32_t returnAddress, std::uint32_t instructionTicks, Args... args) {
  static_assert(sizeof...(Args) <= 4);
  static_assert((std::is_convertible_v<Args, std::uint32_t> && ...));
  const std::uint32_t values[] = {static_cast<std::uint32_t>(args)..., 0u};
  for (std::size_t index = 0; index < sizeof...(Args); ++index) {
    core.r[4 + index] = values[index];
  }
  core.r[31] = returnAddress;
  rec_guest_instruction_ticks(&core, instructionTicks);
  runtime::dispatchGuest(core, target);
  return core.r[2];
}

} // namespace crashbash
