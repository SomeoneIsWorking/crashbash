#pragma once

#include "native_dispatch.h"
#include "run_ledger.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class Core;
namespace psx::state {
class NativeStatePort;
}

namespace crashbash::runtime {

class ImageIdentityState;

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

// One authenticated binding as a whole-machine state records it: the logical image, the catalog
// residency it was published as (name, content identity, published range) and the physical ranges of
// that residency that no later write has retired. Restoring a state re-establishes exactly these
// against the restored guest RAM, so identity, generation and native keys agree with the bytes.
struct BoundImageRecord {
  GuestImage image = GuestImage::Resident;
  std::string name;
  std::uint64_t contentIdentity = 0;
  GuestAddressRange range;
  std::vector<GuestAddressRange> residentRanges;
};

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

  // Every binding with its surviving residency, in publication order (the catalog's own precedence).
  std::vector<BoundImageRecord> boundImages() const;
  // TRUE when every active residency on this Core's catalog is one of this context's bindings, which
  // is what makes replacing the bindings a complete replacement of the Core's image identity.
  bool ownsEveryActiveResidency() const;
  // Replace every binding with `records`, as `boundImages` recorded them: retire the current
  // residencies and their native keys, then publish each record as a FRESH generation in order, bind
  // it, and retire what its record says no longer survives. Only for records that `boundImages`
  // produced on this title; the caller has validated them.
  void restoreBoundImages(const std::vector<BoundImageRecord> &records);

  // The one whole-run ledger for this Core. It is a member because this context is the Core's own
  // title state: it is created with the Core, destroyed with it, and every guest-execution fact the
  // ledger prints is observed here. It is a reference, not an owned copy, so the ledger the product
  // reports is provably the one this execution fed.
  diagnostics::RunLedger &ledger() {
    return ledger_;
  }
  const diagnostics::RunLedger &ledger() const {
    return ledger_;
  }
  // This Core's native state in a whole-machine save state (image_identity_state.h).
  psx::state::NativeStatePort &statePort();

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
  // Subtract `physicalRange` from one binding's residency and retire its native keys inside it.
  // Returns false when nothing of the residency survives; its owners are then already removed and the
  // caller erases the binding.
  bool retireBindingRange(const Binding &binding, GuestAddressRange physicalRange);
  // The run ledger is told the registration and binding counts here, where they change, so its
  // run-end line has a producer instead of reading title state something else may have moved.
  void noteOwnershipCensus();

  Core &core_;
  diagnostics::RunLedger ledger_;
  std::vector<Registration> registrations_;
  std::vector<Binding> bindings_;
  std::unique_ptr<ImageIdentityState> statePort_;
};

// The ledger belonging to the Core's own title context. psxport installs the context from `Core`'s
// constructor, BEFORE the product has a Core to build a ledger for, so the context cannot be handed
// one at construction, and it is the only per-Core handle the framework gives: a native override
// receives a bare `Core *`, and a CD-read completion has a signature the framework owns. This
// returns the ledger of THAT Core's context — a lookup of a member, not a singleton.
diagnostics::RunLedger &runLedgerFor(Core &core);

// Retained native owners all use this same context and shared dispatcher.
void registerNativeOverride(
    Core &core, GuestImage image, std::uint32_t address, std::string_view name, NativeOverride function);
void retireAuthenticatedImagesForWrite(Core &core, GuestAddressRange physicalRange);

// Enter ordinary guest code through the runtime dispatcher. Callers establish the measured guest
// ABI state (including r31 and instruction timing) before entering this boundary.
void dispatchGuest(Core &core, std::uint32_t address);
psx::cpu::ExecutionResult dispatchGuestSlice(Core &core, std::uint32_t address, psx::cpu::ExecutionBudget budget);

// The turn cap for a Crash Bash guest or original call, in display fields.
//
// One display field is 564,480 guest cycles, so this is "a call may span N display fields of guest
// CPU and no more". The worst measured case in this title is the full-frame 15-bit channel swap
// entered from a native override (MENU entry 0x800B5244 took 7 turns / 6.097 fields; the BOOT logo
// update 0x8008E5BC took 6 turns / 5.677 fields). Both convert a 256 KiB (128-sector) image, the
// largest single conversion this title's load path performs, so 12 leaves room for a 2x larger
// image at the same per-pixel cost and still reports a real guest loop within 12 host turns (0.2 s).
//
// This is a TITLE value, not the framework's: `psx::cpu::kDefaultCallTurns` is 8, which is below
// this title's measured worst case, so every call site states this one. The deepest turn any
// completed call needed is printed with the census at run end, so it is falsifiable from a log
// rather than trusted.
inline constexpr std::uint32_t kTitleCallTurnCap = 12u;

// One guest or original call carried to its return address across as many bounded host turns as it
// genuinely needs, and NOT one more.
//
// The loop, the captured-and-latched return address, the no-progress refusal and the
// Returned/Suspended/Refused classification are psx::cpu's (`ResumableGuestCall`). What stays here
// is this title's part: `kTitleCallTurnCap`, the resumed-original image-generation check, and what a
// refusal MEANS for a Crash Bash run — this port refuses the run through its ledger rather than
// aborting, so a bounded call that never returns is reported with the rest of the run's facts.
//
// `returnPc` is the caller's return address as the FIRST turn saw it, captured before the dispatch:
// a resume must not adopt the nested `$ra` the guest left behind. `original` names the native key the
// first turn suppressed, which is the only difference between psx::cpu::resumeOriginal and
// resumeGuestToReturn and is what keeps a resumed original from re-entering its own override.
// Returns the guest's r[2]. A refusal is reported through this title's ledger and refuses the run,
// so there is no meaningful value in that case and 0 is returned.
std::uint32_t runGuestCallToReturn(Core &core,
                                   std::uint32_t entry,
                                   std::uint32_t returnPc,
                                   std::string_view owner,
                                   const std::optional<psx::cpu::NativeKey> &original);

// This title's own turn-cap line beside psxport's census line: the cap value is a title fact, and
// the census does not know it.
void reportGuestCallTurnCap(Core &core, std::string_view why);

// Execute the authenticated guest body for the currently active override through Lightrec while
// suppressing only that override. This is the sole replacement for generated "super" bodies, and it
// runs to the call's return address under the same fence as any other guest call.
void callOriginal(Core &core, GuestImage image, std::uint32_t address);

} // namespace crashbash::runtime
