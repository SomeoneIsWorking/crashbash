#include "guest_execution.h"

#include "core.h"
#include "execution_control.h"
#include "image_identity.h"
#include "image_identity_state.h"
#include "lightrec_executor.h"
#include "native_dispatch.h"
#include "run_ledger.h"

#include <algorithm>
#include <cstdlib>
#include <lucent/log.h>
#include <stdexcept>

namespace crashbash::runtime {
namespace {

GuestExecution &execution(Core &core) {
  if (!core.gameCtx) {
    lucent::error("crashbash-runtime", "guest execution requires the title's per-Core context");
    std::abort();
  }
  return *static_cast<GuestExecution *>(core.gameCtx);
}

// The fence on a resume, in the unit that gives it meaning. One display field is 564,480 guest
// cycles, so this is "a call may span N display fields of guest CPU and no more".
//
// MEASURED in this title over 400 native frames (`PSXPORT_NATIVE_FRAMES=400`, log in
// scratch/afterfix/run.log): 30,751 guest calls completed, and exactly 3 of them outlived a host
// turn. All three are the same work — a full-frame 15-bit channel swap entered from a native
// override — and they bracket it: MENU entry 0x800B5244 took 7 turns / 3,441,400 cycles / 6.097
// fields, and the BOOT logo update 0x8008E5BC took 6 turns / 3,204,494 cycles / 5.677 fields twice.
// Both convert a 256 KiB (128-sector) image, the largest single conversion this title's own load
// path performs, so 12 leaves room for a 2x larger image at the same per-pixel cost and still
// reports a real guest loop within 12 host turns (0.2 s) instead of spinning on it. The deepest turn
// any completed call needed is printed with the census at run end, so this number is falsifiable
// from a log, not trusted.
constexpr std::uint32_t kGuestCallTurnCap = 12u;

// Run-lifetime diagnostic tally, NOT execution state: the resume loop keeps no cross-call record and
// nothing in the execution path reads or writes a Core through it. It exists so the run-end line can
// name a denominator. This product creates exactly one Core (game/core/player_entry.cpp).
struct GuestCallCensus {
  std::uint64_t completed = 0;
  std::uint64_t resumed = 0;
  std::uint32_t deepestTurns = 0;
  std::uint64_t resumedCycles = 0;
};

GuestCallCensus census;

double displayFields(const Core &core, std::uint64_t cycles) {
  return static_cast<double>(cycles) / static_cast<double>(psx::cpu::ExecutionBudget::currentTurn(core).cycles);
}

// The logical name a tracked image is known by everywhere else in this title. Named here, next to
// the enum it names, so the ledger and the refusals cannot spell one image two ways.
const char *imageName(GuestImage image) {
  switch (image) {
  case GuestImage::Resident:
    return "resident";
  case GuestImage::Boot:
    return "boot";
  case GuestImage::Menu:
    return "menu";
  case GuestImage::Dat28136:
    return "dat28136";
  case GuestImage::Dat22510:
    return "dat22510";
  case GuestImage::Dat28272:
    return "dat28272";
  case GuestImage::Dat28241:
    return "dat28241";
  case GuestImage::Dat28382:
    return "dat28382";
  }
  return "unknown";
}

void recordCompleted(
    Core &core, std::uint32_t entry, std::uint32_t returnPc, std::uint32_t turns, std::uint64_t cycles) {
  ++census.completed;
  if (turns <= 1u) {
    return;
  }
  ++census.resumed;
  census.resumedCycles += cycles;
  if (turns > census.deepestTurns) {
    census.deepestTurns = turns;
  }
  lucent::info("crashbash-guest",
               "guest call 0x{:08X} to return address 0x{:08X} outlived one host turn: {} turn(s), "
               "{} guest cycles ({:.3f} display fields). Denominator: {} of {} completed guest call(s) "
               "have needed a resume; deepest {} turn(s)",
               entry,
               returnPc,
               turns,
               cycles,
               displayFields(core, cycles),
               census.resumed,
               census.completed,
               census.deepestTurns);
}

} // namespace

GuestExecution::GuestExecution(Core &core)
    : core_(core), ledger_(core), statePort_(std::make_unique<ImageIdentityState>(*this)) {
  if (core.gameCtx) {
    throw std::logic_error("Crash Bash Core already has a title context");
  }
  core.gameCtx = this;
}

GuestExecution::~GuestExecution() {
  for (const auto &binding : bindings_) {
    removeRegistrations(binding);
  }
  core_.gameCtx = nullptr;
}

void GuestExecution::removeRegistrations(const Binding &binding) {
  for (const auto &registration : registrations_) {
    if (registration.image == binding.image) {
      core_.nativeDispatcher().remove({binding.identity, registration.address});
    }
  }
}

void GuestExecution::registerOverride(GuestImage image,
                                      std::uint32_t address,
                                      std::string_view name,
                                      NativeOverride function) {
  if (!function || name.empty() || (address & 3u) != 0u) {
    throw std::invalid_argument("Crash Bash native override is incomplete or unaligned");
  }
  if (std::any_of(registrations_.begin(), registrations_.end(), [=](const Registration &entry) {
        return entry.image == image && entry.address == address;
      })) {
    throw std::invalid_argument("Crash Bash native override is already registered");
  }
  const auto binding = std::find_if(bindings_.begin(), bindings_.end(), [image](const Binding &entry) {
    return entry.image == image;
  });
  if (binding != bindings_.end()) {
    if (!binding->range.containsPhysical(address) || core_.currentImageIdentity(address) != binding->identity) {
      throw std::invalid_argument("Crash Bash override does not belong to the active authenticated image");
    }
    if (!core_.nativeDispatcher().install({{binding->identity, address}, name, function})) {
      throw std::logic_error("Crash Bash native override conflicts with an existing runtime owner");
    }
  }
  registrations_.push_back({image, address, std::string(name), function});
  noteOwnershipCensus();
}

void GuestExecution::bindAuthenticatedImage(GuestImage image,
                                            psx::cpu::ImageIdentity identity,
                                            GuestAddressRange range) {
  if (core_.currentImageIdentity(range) != identity || identity.id == 0u || identity.generation == 0u) {
    throw std::invalid_argument("Crash Bash image binding requires one complete active authenticated residency");
  }
  for (const auto &binding : bindings_) {
    if (binding.identity == identity) {
      throw std::invalid_argument("Crash Bash image generation is already bound");
    }
  }
  for (const auto &registration : registrations_) {
    if (registration.image != image) {
      continue;
    }
    if (!range.containsPhysical(registration.address) ||
        core_.nativeDispatcher().isInstalled({identity, registration.address})) {
      throw std::invalid_argument("Crash Bash image cannot bind its registered native owners");
    }
  }
  // Validate the entire replacement before removing any previously published native owner.
  unbindImage(image);
  const Binding binding{image, identity, range};
  bindings_.push_back(binding);
  noteOwnershipCensus();
  for (const auto &registration : registrations_) {
    if (registration.image == image &&
        !core_.nativeDispatcher().install(
            {{identity, registration.address}, registration.name, registration.function})) {
      lucent::error("crashbash-runtime", "validated image binding failed to install a native owner");
      std::abort();
    }
  }
}

void GuestExecution::unbindImage(GuestImage image) {
  const auto binding = std::find_if(bindings_.begin(), bindings_.end(), [image](const Binding &entry) {
    return entry.image == image;
  });
  if (binding != bindings_.end()) {
    removeRegistrations(*binding);
    bindings_.erase(binding);
    noteOwnershipCensus();
  }
}

bool GuestExecution::retireBindingRange(const Binding &binding, GuestAddressRange physicalRange) {
  const auto remaining = core_.imageCatalog().subtractRange(binding.identity, physicalRange);
  if (remaining == 0u) {
    removeRegistrations(binding);
    return false;
  }
  for (const auto &registration : registrations_) {
    if (registration.image == binding.image && physicalRange.containsPhysical(registration.address)) {
      core_.nativeDispatcher().remove({binding.identity, registration.address});
    }
  }
  return true;
}

void GuestExecution::retireImagesOverlapping(GuestAddressRange physicalRange) {
  if (!physicalRange.valid() || physicalRange.end > sizeof(core_.ram)) {
    throw std::invalid_argument("Crash Bash loaded-image retirement requires one physical range");
  }
  for (auto binding = bindings_.begin(); binding != bindings_.end();) {
    if (binding->range.end <= physicalRange.begin || binding->range.begin >= physicalRange.end) {
      ++binding;
      continue;
    }
    if (!retireBindingRange(*binding, physicalRange)) {
      binding = bindings_.erase(binding);
      noteOwnershipCensus();
      continue;
    }
    ++binding;
  }
}

std::vector<BoundImageRecord> GuestExecution::boundImages() const {
  std::vector<const Binding *> ordered;
  ordered.reserve(bindings_.size());
  for (const auto &binding : bindings_) {
    ordered.push_back(&binding);
  }
  std::sort(ordered.begin(), ordered.end(), [](const Binding *left, const Binding *right) {
    return left->identity.generation < right->identity.generation;
  });
  std::vector<BoundImageRecord> records;
  records.reserve(ordered.size());
  for (const Binding *binding : ordered) {
    const auto description = core_.imageCatalog().describe(binding->identity);
    if (!description) {
      throw std::logic_error("Crash Bash binding names an image generation its catalog never published");
    }
    records.push_back(
        {binding->image, description->name, description->contentIdentity, binding->range, description->ranges});
  }
  return records;
}

psx::state::NativeStatePort &GuestExecution::statePort() {
  return *statePort_;
}

bool GuestExecution::ownsEveryActiveResidency() const {
  return core_.imageCatalog().activeCount() == bindings_.size();
}

void GuestExecution::restoreBoundImages(const std::vector<BoundImageRecord> &records) {
  for (const auto &binding : bindings_) {
    removeRegistrations(binding);
    core_.imageCatalog().deactivate(binding.identity);
  }
  bindings_.clear();
  noteOwnershipCensus();
  for (const BoundImageRecord &record : records) {
    const psx::cpu::ImageIdentity identity =
        core_.imageCatalog().activate(record.name, record.range, record.contentIdentity);
    bindAuthenticatedImage(record.image, identity, record.range);
    // The record's surviving ranges are sorted and disjoint inside its range; every gap between them
    // is a span a later write retired before the state was taken.
    std::uint32_t cursor = record.range.begin;
    std::vector<GuestAddressRange> retired;
    for (const GuestAddressRange surviving : record.residentRanges) {
      if (cursor < surviving.begin) {
        retired.push_back({cursor, surviving.begin});
      }
      cursor = surviving.end;
    }
    if (cursor < record.range.end) {
      retired.push_back({cursor, record.range.end});
    }
    const Binding binding{record.image, identity, record.range};
    for (const GuestAddressRange gap : retired) {
      if (!retireBindingRange(binding, gap)) {
        throw std::logic_error("Crash Bash restored image record had no surviving residency");
      }
    }
  }
}

void GuestExecution::noteOwnershipCensus() {
  ledger_.noteInstalledOverrides(registrations_.size(), bindings_.size());
}

std::optional<psx::cpu::NativeKey> GuestExecution::activeKey(GuestImage image, std::uint32_t address) const {
  for (const auto &binding : bindings_) {
    if (binding.image == image && binding.range.containsPhysical(address) &&
        core_.currentImageIdentity(address) == binding.identity) {
      return psx::cpu::NativeKey{binding.identity, address};
    }
  }
  return std::nullopt;
}

psx::cpu::ExecutionResult
GuestExecution::original(GuestImage image, std::uint32_t address, psx::cpu::ExecutionBudget budget) {
  const auto key = activeKey(image, address);
  if (!key || !core_.nativeDispatcher().isInstalled(*key)) {
    return {psx::cpu::ExecutionExitReason::Fault,
            address,
            0,
            "Crash Bash original call has no native owner in the current authenticated image generation"};
  }
  return psx::cpu::callOriginal(core_, *key, budget);
}

diagnostics::RunLedger &runLedgerFor(Core &core) {
  return execution(core).ledger();
}

void registerNativeOverride(
    Core &core, GuestImage image, std::uint32_t address, std::string_view name, NativeOverride function) {
  execution(core).registerOverride(image, address, name, function);
}

void retireAuthenticatedImagesForWrite(Core &core, GuestAddressRange physicalRange) {
  execution(core).retireImagesOverlapping(physicalRange);
}

void dispatchGuest(Core &core, std::uint32_t address) {
  const psx::cpu::ExecutionResult result =
      dispatchGuestSlice(core, address, psx::cpu::ExecutionBudget::currentTurn(core));
  if (!psx::cpu::requireGuestReturn(result, "Crash Bash guest call")) {
    diagnostics::RunLedger &ledger = execution(core).ledger();
    ledger.noteGuestFault("Crash Bash guest call", result.guestPc, result.detail);
    ledger.refuseRun(lucent::format("the guest call entered at 0x{:08X} did not return", address));
  }
}

psx::cpu::ExecutionResult dispatchGuestSlice(Core &core, std::uint32_t address, psx::cpu::ExecutionBudget budget) {
  return psx::cpu::dispatchGuest(core, address, budget);
}

psx::cpu::ExecutionResult runGuestCallToReturn(Core &core,
                                               std::uint32_t entry,
                                               std::uint32_t returnPc,
                                               std::string_view owner,
                                               const std::optional<psx::cpu::NativeKey> &original,
                                               psx::cpu::ExecutionResult result) {
  std::uint64_t cycles = result.cycles;
  std::uint32_t turns = 1u;
  while (result.reason == psx::cpu::ExecutionExitReason::BudgetExhausted) {
    if (result.cycles == 0u || result.guestPc == 0u) {
      lucent::error("crashbash-guest",
                    "{}: guest call 0x{:08X} to return address 0x{:08X} exhausted host turn {} with no "
                    "guest progress at 0x{:08X} after {} cycles: {}",
                    owner,
                    entry,
                    returnPc,
                    turns,
                    result.guestPc,
                    cycles,
                    result.detail);
      execution(core).ledger().refuseRun("a guest call exhausted its host turn with no guest progress");
    }
    if (turns >= kGuestCallTurnCap) {
      lucent::error("crashbash-guest",
                    "{}: guest call 0x{:08X} to return address 0x{:08X} has consumed {} host turn(s) and "
                    "{} cycles ({:.3f} display fields) without reaching its return address and is still at "
                    "0x{:08X}: {}. A bounded guest call that needs more than {} display fields does not "
                    "exist in this run, so this is a guest loop — reported rather than spun on",
                    owner,
                    entry,
                    returnPc,
                    turns,
                    cycles,
                    displayFields(core, cycles),
                    result.guestPc,
                    result.detail,
                    kGuestCallTurnCap);
      execution(core).ledger().refuseRun("a guest call ran past the host-turn cap without returning");
    }
    // A resumed ORIGINAL re-establishes the suppression of its own override from the key. If the image
    // generation behind that key is no longer the one mapped at the entry, the suppression would
    // silently apply to nothing and the guest would re-enter the override it is running inside. Every
    // nested slot in this title is reused by a later module, so this is a reachable state, not a
    // theoretical one: refuse it loudly instead of resuming into it.
    if (original) {
      const auto active = core.currentImageIdentity(entry);
      if (!active || *active != original->image) {
        lucent::error("crashbash-guest",
                      "{}: guest call 0x{:08X} outlived its host turn, but the authenticated image "
                      "generation behind it is no longer mapped at that address. Its override is no "
                      "longer suppressed, so resuming would re-enter it from inside the original body",
                      owner,
                      entry);
        execution(core).ledger().refuseRun("a resumed original call lost the image generation behind its override");
      }
    }
    const psx::cpu::ExecutionBudget turn = psx::cpu::ExecutionBudget::currentTurn(core);
    result = original ? psx::cpu::resumeOriginal(core, *original, result.guestPc, returnPc, turn)
                      : psx::cpu::resumeGuestToReturn(core, result.guestPc, returnPc, turn);
    cycles += result.cycles;
    ++turns;
  }
  if (!psx::cpu::requireGuestReturn(result, owner)) {
    diagnostics::RunLedger &ledger = execution(core).ledger();
    ledger.noteGuestFault(owner, result.guestPc, result.detail);
    ledger.refuseRun("a guest call ended in something other than a return");
  }
  recordCompleted(core, entry, returnPc, turns, cycles);
  return result;
}

void reportGuestCallCensus(std::string_view why) {
  if (census.completed == 0u) {
    lucent::info("crashbash-guest", "run-end ({}): NO guest call completed, so this run measured nothing", why);
    return;
  }
  if (census.resumed == 0u) {
    lucent::info("crashbash-guest",
                 "run-end ({}): {} guest call(s) completed, 0 needed a resume — every call returned inside "
                 "the one display field its host turn allows",
                 why,
                 census.completed);
    return;
  }
  lucent::info("crashbash-guest",
               "run-end ({}): {} guest call(s) completed, {} needed a resume (deepest {} host turn(s) against "
               "a cap of {}, {} guest cycles over those calls); the other {} finished inside one field",
               why,
               census.completed,
               census.resumed,
               census.deepestTurns,
               kGuestCallTurnCap,
               census.resumedCycles,
               census.completed - census.resumed);
}

void callOriginal(Core &core, GuestImage image, std::uint32_t address) {
  GuestExecution &context = execution(core);
  context.ledger().noteOriginalCall(imageName(image), address);
  const std::uint32_t returnPc = core.r[31];
  const auto key = context.activeKey(image, address);
  const auto first = context.original(image, address, psx::cpu::ExecutionBudget::currentTurn(core));
  lucent::debug("crashbash-original",
                "target=0x{:08X} exit={} pc=0x{:08X} cycles={} r4=0x{:08X} r5=0x{:08X} r6=0x{:08X} "
                "r7=0x{:08X} r8=0x{:08X} r16=0x{:08X} r17=0x{:08X} r18=0x{:08X} ra=0x{:08X}",
                address,
                psx::cpu::executionExitName(first.reason),
                first.guestPc,
                first.cycles,
                core.r[4],
                core.r[5],
                core.r[6],
                core.r[7],
                core.r[8],
                core.r[16],
                core.r[17],
                core.r[18],
                core.r[31]);
  runGuestCallToReturn(core, address, returnPc, "Crash Bash original call", key, first);
}

} // namespace crashbash::runtime
