#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class Core;

namespace crashbash::diagnostics {

// How the run ended. `Refused` is not a rare corner: it is the case whose numbers nobody has, so the
// ledger is emitted on it exactly as it is on a clean finish.
//
// WHAT THIS LEDGER IS NOT. It does not report the dynarec's own numbers. psxport's
// `runtime/cpu/execution_ledger.h` prints translated and executed blocks and instructions, cache
// hits and misses, invalidations by source, the invalidation work, budget exits and every fallback
// reason from the same `ExecutorCounters`, on the `guest` channel at every run end. This owner
// therefore reports only what the framework has no counter for: WHICH authenticated image was
// published, from which offer, with what witnesses, and what a title's own refusals, faults, native
// registrations and original-body calls were. The execution numbers are CONSUMED (the zero-work
// warning and each publication's invalidation delta) and never re-derived or reprinted.
enum class RunOutcome {
  Completed,
  Refused,
};

// One authenticated image this run published, with the invalidation the publication caused.
//
// The invalidation and translation counts are read from the framework's OWN executor counters as
// deltas across the publication, so this record cannot claim a range was retired when the runtime
// retired nothing: the number is the runtime's, taken immediately before and after.
struct ImagePublication {
  std::string logicalImage; // the tracked module, e.g. "boot" / "menu" / "dat28272"
  std::string name;         // the authenticated image name the catalog carries
  std::uint64_t generation = 0;
  std::uint32_t physicalBegin = 0;
  std::uint32_t physicalEnd = 0;
  std::uint32_t sectors = 0;
  std::size_t bytes = 0;
  int contentWitnesses = 0;
  // True when this publication came from the CD publication boundary, so it is one of the loads the
  // offer census counts. The resident executable is published by the player's own loader and is NOT
  // one of them, and folding it into that census would make a run that published only the resident
  // read as a matched load nobody published.
  bool fromPublicationBoundary = false;
  // How many authenticated residencies were ACTIVE when this load was published. It is the context
  // for `invalidations`: a publication that replaced the only other resident must invalidate, and
  // one that extended an empty catalog need not.
  std::size_t residenciesBefore = 0;
  std::uint64_t invalidations = 0; // framework counter delta across the publication
  std::uint64_t blocksTranslated = 0;
};

// A load the publication boundary was offered and MATCHED, whatever the outcome. This is the
// denominator the publication census is meaningless without: a census of publications alone cannot
// say whether a matched load went unpublished, because "published nothing" and "was offered
// nothing" are the same zero.
struct ModuleOffer {
  std::string logicalImage;
  std::uint32_t discLba = 0;
  std::uint32_t destination = 0;
  std::uint32_t sectors = 0;
};

// The guest-call census. One entry is one guest or original call carried to its return address; the
// resume counters are how many of those outlived the single display field one host turn allows.
struct GuestCallCensus {
  std::uint64_t completed = 0;
  std::uint64_t resumed = 0;
  std::uint32_t deepestTurns = 0;
  std::uint64_t resumedCycles = 0;
};

// The measured facts, exposed so a test can assert on the census the report prints instead of on
// the log text.
struct RunLedgerFacts {
  std::vector<ImagePublication> publications;
  std::vector<ModuleOffer> offers;
  std::vector<std::string> refusals;
  std::vector<std::string> residentRefusals;
  std::vector<std::string> faults;
  std::size_t faultsDropped = 0;
  // Faults the runtime itself identified as a REFUSED fallback, counted by its own fault text
  // ("Lightrec fallback refused before interpreter execution: reason=..."). That string is the only
  // place a fallback reason is published to a port, so recognising it here is reading the runtime's
  // own classification rather than re-deriving one; anything else is counted as an ordinary fault.
  std::size_t fallbackRefusalFaults = 0;
  std::size_t originalCalls = 0;
  std::size_t registrations = 0;
  std::size_t boundImages = 0;
  std::vector<std::string> originalCallSites;
  GuestCallCensus guestCalls;
};

// The whole-run image, refusal and native-ownership ledger for ONE Core.
//
// It is an owner, not a namespace of process state: the facts, the Core they are measured against
// and the once-only report all live in one instance with a bounded lifetime, and every feeder is a
// member call on the instance the caller already holds. Nothing here is static, so two Cores cannot
// share a census and a run cannot outlive its ledger — the destructor reports whatever the run had
// not yet reported.
//
// Construction IS arming. There is no separate arm step, so "armed for the wrong Core" and "armed
// twice" are not states this type can be in.
class RunLedger final {
public:
  explicit RunLedger(Core &core);
  ~RunLedger();
  RunLedger(const RunLedger &) = delete;
  RunLedger &operator=(const RunLedger &) = delete;
  RunLedger(RunLedger &&) = delete;
  RunLedger &operator=(RunLedger &&) = delete;

  // Report this run's outcome, once. A second call for one instance is refused with an error line
  // rather than duplicated: two run-end ledgers for one run are two denominators for one
  // measurement. The destructor calls this for whatever is still unreported, so the ordinary end of
  // a run — a frame budget, a recorded scenario, a debug-endpoint stop — is covered without the
  // caller remembering.
  void close(std::string_view why);
  void report(RunOutcome outcome, std::string_view why);

  // This product's ONE refusal route: report the ledger for the run being refused, then abort.
  // `std::abort()` does not unwind, so a destructor cannot report on this path, and a run that dies
  // is precisely the run whose image census and refusal sites are missing.
  [[noreturn]] void refuseRun(std::string_view why);

  // Fact feeders. Each is called where the fact happens, so the run-end numbers have a visible
  // producer rather than a reader with nothing behind it.
  void noteImagePublication(const ImagePublication &publication);
  void noteModuleOffer(const ModuleOffer &offer);
  void noteModuleRefusal(std::string_view logicalImage, std::string_view why);
  // A refusal of the player's own executable load. It is NOT a module offer — nothing was matched
  // and nothing reached guest RAM — so counting it against the offer census would report an identity
  // gap for bytes that never existed in RAM.
  void noteResidentRefusal(std::string_view why);
  void noteOriginalCall(std::string_view logicalImage, std::uint32_t address);
  void noteInstalledOverrides(std::size_t registrations, std::size_t boundImages);
  // A guest call that ended in something other than a return, at the PC the executor reported and
  // with the runtime's own words for why. This is the ONLY place a fallback site reaches a port:
  // the runtime admits an admitted fallback inside one `lightrec_execute` call and hands back an
  // ordinary result, and refuses a refused one as a typed fault carrying the offending guest PC.
  void noteGuestFault(std::string_view owner, std::uint32_t guestPc, std::string_view detail);
  // One completed guest call, with the host turns it took and the guest cycles it spent. The
  // resume fence's denominator comes from here rather than from a tally beside the resume loop.
  void noteGuestCall(std::uint32_t turns, std::uint64_t cycles);

  const RunLedgerFacts &facts() const {
    return facts_;
  }
  bool reported() const {
    return reported_;
  }
  Core &core() const {
    return core_;
  }

private:
  void reportExecutionContext() const;
  void reportImages() const;
  void reportNativeOwnership() const;

  Core &core_;
  RunLedgerFacts facts_;
  bool reported_ = false;
};

} // namespace crashbash::diagnostics