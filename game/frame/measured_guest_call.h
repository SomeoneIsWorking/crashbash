#pragma once

#include "core.h"
#include "execution_exit.h"
#include "execution_services.h"
#include "guest_execution.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace crashbash {

// One owner for the guest-call mechanics used by the RE-derived boot and frame modules. The return
// address and instruction count at each call site come from the emitted retail body.
// Establishes the measured guest ABI for a call — the return address and the argument registers the
// emitted retail body set, plus the instruction timing it costs — WITHOUT dispatching it. The
// dispatch itself is `psx::cpu::ResumableGuestCall`'s, so a call that outlives one host turn is
// resumed by the shared owner rather than by this title.
template <typename... Args>
void measuredGuestCallSetup(Core &core, std::uint32_t returnAddress, std::uint32_t instructionTicks, Args... args) {
  static_assert(sizeof...(Args) <= 4);
  static_assert((std::is_convertible_v<Args, std::uint32_t> && ...));
  const std::uint32_t values[] = {static_cast<std::uint32_t>(args)..., 0u};
  for (std::size_t index = 0; index < sizeof...(Args); ++index) {
    core.r[4 + index] = values[index];
  }
  core.r[31] = returnAddress;
  psx::cpu::accountGuestInstructions(core, instructionTicks);
}

template <typename... Args>
psx::cpu::ExecutionResult measuredGuestCallSlice(Core &core,
                                                 std::uint32_t target,
                                                 std::uint32_t returnAddress,
                                                 std::uint32_t instructionTicks,
                                                 psx::cpu::ExecutionBudget budget,
                                                 Args... args) {
  measuredGuestCallSetup(core, returnAddress, instructionTicks, args...);
  return runtime::dispatchGuestSlice(core, target, budget);
}

template <typename... Args>
std::uint32_t measuredGuestCall(
    Core &core, std::uint32_t target, std::uint32_t returnAddress, std::uint32_t instructionTicks, Args... args) {
  // `measuredGuestCallSetup` publishes `returnAddress` as the caller's r[31], which is exactly what
  // the shared call latches as its return boundary, so the resume boundary is the MEASURED return
  // address rather than the nested `$ra` a resumed turn would otherwise leave behind.
  measuredGuestCallSetup(core, returnAddress, instructionTicks, args...);
  return runtime::runGuestCallToReturn(core, target, returnAddress, "Crash Bash guest call", std::nullopt);
}

} // namespace crashbash
