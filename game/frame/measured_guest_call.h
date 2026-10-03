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
template <typename... Args>
psx::cpu::ExecutionResult measuredGuestCallSlice(Core &core,
                                                 std::uint32_t target,
                                                 std::uint32_t returnAddress,
                                                 std::uint32_t instructionTicks,
                                                 psx::cpu::ExecutionBudget budget,
                                                 Args... args) {
  static_assert(sizeof...(Args) <= 4);
  static_assert((std::is_convertible_v<Args, std::uint32_t> && ...));
  const std::uint32_t values[] = {static_cast<std::uint32_t>(args)..., 0u};
  for (std::size_t index = 0; index < sizeof...(Args); ++index) {
    core.r[4 + index] = values[index];
  }
  core.r[31] = returnAddress;
  psx::cpu::accountGuestInstructions(core, instructionTicks);
  return runtime::dispatchGuestSlice(core, target, budget);
}

template <typename... Args>
std::uint32_t measuredGuestCall(
    Core &core, std::uint32_t target, std::uint32_t returnAddress, std::uint32_t instructionTicks, Args... args) {
  // `measuredGuestCallSlice` publishes `returnAddress` as the caller's r[31], which is exactly what
  // psx::cpu::dispatchGuest stops at, so the resume boundary is the MEASURED return address rather than
  // the nested `$ra` a resumed turn would otherwise leave behind.
  runtime::runGuestCallToReturn(
      core,
      target,
      returnAddress,
      "Crash Bash guest call",
      std::nullopt,
      measuredGuestCallSlice(
          core, target, returnAddress, instructionTicks, psx::cpu::ExecutionBudget::currentTurn(core), args...));
  return core.r[2];
}

} // namespace crashbash
