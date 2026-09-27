#pragma once

#include "native_dispatch.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class Core;

namespace crashbash::runtime {

// Runtime image identity is part of every override/cache key because Crash Bash reuses guest
// address ranges for unrelated loaded modules. The psxport adapter resolves these logical images
// to the authenticated image generation currently mapped by the runtime.
enum class GuestImage {
  Resident,
  Boot,
  Menu,
  Dat28136,
  Dat22510,
  // The remaining tracked occupants of the reused 0x800B32B4 nested slot. They are named here
  // because a logical image that has no value cannot be named by a native key, and a native key
  // that cannot be named is an override or a translated block with no identity to retire.
  Dat28272,
  Dat28241,
  Dat28382,
};

using NativeOverride = void (*)(Core *);

// Per-Core title context. The loader authenticates bytes and activates their shared ImageCatalog
// residency before binding here, and unbinds before unloading. This owner never authenticates an
// address or digest by inference and never creates a second executable-image catalog.
class GuestExecution final {
public:
  explicit GuestExecution(Core &core);
  ~GuestExecution();
  GuestExecution(const GuestExecution &) = delete;
  GuestExecution &operator=(const GuestExecution &) = delete;
  GuestExecution(GuestExecution &&) = delete;
  GuestExecution &operator=(GuestExecution &&) = delete;

  std::size_t registeredOverrideCount() const {
    return registrations_.size();
  }

  void registerOverride(GuestImage image, std::uint32_t address, std::string_view name, NativeOverride function);
  void bindAuthenticatedImage(GuestImage image, psx::cpu::ImageIdentity identity, GuestAddressRange range);
  void unbindImage(GuestImage image);
  // External writes remove only the overwritten bytes from an authenticated generation and retire
  // native keys at overwritten entries. Unchanged fragments retain their original identity.
  void retireImagesOverlapping(GuestAddressRange physicalRange);
  std::optional<psx::cpu::NativeKey> activeKey(GuestImage image, std::uint32_t address) const;
  // ONE bounded host turn of the original body for this override key. It reports that turn's typed
  // exit unchanged: BudgetExhausted is an ordinary outcome, and what to do about it belongs to
  // runGuestCallToReturn below, not to a call site.
  psx::cpu::ExecutionResult original(GuestImage image, std::uint32_t address, psx::cpu::ExecutionBudget budget);

private:
  struct Registration {
    GuestImage image;
    std::uint32_t address;
    std::string name;
    NativeOverride function;
  };
  struct Binding {
    GuestImage image;
    psx::cpu::ImageIdentity identity;
    GuestAddressRange range;
  };
  void removeRegistrations(const Binding &binding);

  Core &core_;
  std::vector<Registration> registrations_;
  std::vector<Binding> bindings_;
};

// Retained native owners all use this same context and shared dispatcher.
void registerNativeOverride(
    Core &core, GuestImage image, std::uint32_t address, std::string_view name, NativeOverride function);
void retireAuthenticatedImagesForWrite(Core &core, GuestAddressRange physicalRange);

// Enter ordinary guest code through the runtime dispatcher. Callers establish the measured guest
// ABI state (including r31 and instruction timing) before entering this boundary.
void dispatchGuest(Core &core, std::uint32_t address);
psx::cpu::ExecutionResult dispatchGuestSlice(Core &core, std::uint32_t address, psx::cpu::ExecutionBudget budget);

// One guest or original call carried to its return address across as many bounded host turns as it
// genuinely needs, and NOT one more.
//
// WHY. `psx::cpu::ExecutionBudget::currentTurn` is ONE display field by construction
// (33'868'800/60 = 564,480 cycles, execution_exit.cpp), and the executor contract makes exceeding it
// an ORDINARY bounded exit that host code "commits and handles, then resumes deliberately"
// (psxport/AGENTS.md Executor contract; docs/faithful-execution.md). So a finite guest function that
// needs more than a field is resumed, not aborted: the measured case in this title is the MENU
// image's full-frame 15-bit channel swap, whose loop body is only `lhu`/`sh` on RAM.
//
// `returnPc` is the caller's return address as the FIRST turn saw it, captured before the dispatch:
// a resume must not adopt the nested `$ra` the guest left behind, which is a different address and
// would end the call in the wrong place. `original` names the native key the first turn suppressed,
// if any — that is the only difference between psx::cpu::resumeOriginal and resumeGuestToReturn,
// and it is what keeps a resumed original from re-entering its own override.
//
// A resume must not become a hang, so the loop is fenced by what it can MEASURE:
//   1. a turn that exhausted its budget having consumed no guest cycles, and left no guest PC, made
//      no progress, so resuming it could only repeat that segment — a fact about the exit;
//   2. a call that has spent kGuestCallTurnCap display fields of guest CPU without reaching its
//      return address is a guest loop, not finite compute. That half IS a policy, so it is stated
//      in display fields, set from the measured worst case in this title, and reported per call.
psx::cpu::ExecutionResult runGuestCallToReturn(Core &core,
                                               std::uint32_t entry,
                                               std::uint32_t returnPc,
                                               std::string_view owner,
                                               const std::optional<psx::cpu::NativeKey> &original,
                                               psx::cpu::ExecutionResult first);

// The run's guest-call census: calls completed, how many of those needed a resume, the deepest call
// in host turns, and the guest CPU those calls spent. Printed once the host loop is done, so a run in
// which nothing was ever resumed says so with its denominator instead of leaving silence.
void reportGuestCallCensus(std::string_view why);

// Execute the authenticated guest body for the currently active override through Lightrec while
// suppressing only that override. This is the sole replacement for generated "super" bodies, and it
// runs to the call's return address under the same fence as any other guest call.
void callOriginal(Core &core, GuestImage image, std::uint32_t address);

} // namespace crashbash::runtime
