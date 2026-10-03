#include "run_ledger.h"

#include "core.h"
#include "lightrec_executor.h"

#include <algorithm>
#include <cstdlib>
#include <lucent/log.h>
#include <utility>

namespace crashbash::diagnostics {
namespace {

const char *outcomeName(RunOutcome outcome) {
  return outcome == RunOutcome::Refused ? "REFUSED" : "completed";
}

std::string hex32(std::uint32_t value) {
  std::string text = "0x";
  for (int shift = 28; shift >= 0; shift -= 4) {
    text += "0123456789ABCDEF"[(value >> shift) & 0xFu];
  }
  return text;
}

} // namespace

RunLedger::RunLedger(Core &core) : core_(core) {}

RunLedger::~RunLedger() {
  // The ordinary end of a run reports here. A run that refused already reported and aborted, which
  // never reaches this line, so the once-only guard is the whole of the duplication question.
  if (!reported_) {
    report(RunOutcome::Completed, "the Core was destroyed with this run unreported");
  }
}

void RunLedger::close(std::string_view why) {
  if (!reported_) {
    report(RunOutcome::Completed, why);
  }
}

// What psxport's own run-end ledger (`runtime/cpu/execution_ledger.h`, printed by native_boot) does
// NOT say, and what only this title can say about its own frames.
//
// The execution numbers are deliberately ABSENT here. `logRunEndLedger` already prints translated
// and executed blocks and instructions, cache hits and misses, invalidations by source, the
// invalidation work, budget exits and every fallback reason, from the same `ExecutorCounters` this
// owner reads. Printing them a second time in a second format is the duplicated-code failure the
// framework change removed, and two lines with the same counter and different wording is two answers
// to one question. So this reads the counters it consumes and adds only judgements the framework
// cannot make.
void RunLedger::reportExecutionContext() const {
  const psx::cpu::ExecutorCounters &counters = core_.lightrecExecutor().counters();
  if (counters.executedInstructions == 0u) {
    lucent::error("crashbash-ledger",
                  "run-end: the executor executed 0 guest instruction(s) in {} call(s) and translated {} "
                  "block(s). This run measured NOTHING about dynarec execution: the framework's run-end "
                  "guest line below carries real zeros, not evidence of a healthy runtime",
                  counters.calls,
                  counters.translatedBlocks);
  }
  // The runtime names a fallback's guest PC to a port only when it REFUSES one, and a refusal arrives
  // as a typed guest fault listed with this report. An ADMITTED fallback is inside one
  // `lightrec_execute` call and returns an ordinary result, so no port can see its PC: the
  // framework's fallback line carries the totals, and this line says which half of them has no site
  // list.
  lucent::info("crashbash-ledger",
               "run-end fallback sites: {} refusal site(s) were seen, named by the runtime's own fault "
               "text. An ADMITTED fallback leaves no port-visible site at all, so the framework's "
               "run-end guest ledger carries the totals and nothing else does",
               facts_.fallbackRefusalFaults);
}

void RunLedger::reportImages() const {
  const std::size_t offered = facts_.offers.size();
  const std::size_t refused = facts_.refusals.size();
  std::size_t published = 0u;
  std::size_t residentPublished = 0u;
  std::vector<std::string> exercised;
  for (const ImagePublication &publication : facts_.publications) {
    if (publication.fromPublicationBoundary) {
      ++published;
    } else {
      ++residentPublished;
    }
    if (std::find(exercised.begin(), exercised.end(), publication.logicalImage) == exercised.end()) {
      exercised.push_back(publication.logicalImage);
    }
  }
  for (const std::string &refusal : facts_.refusals) {
    const std::size_t separator = refusal.find(':');
    const std::string image = refusal.substr(0, separator);
    if (std::find(exercised.begin(), exercised.end(), image) == exercised.end()) {
      exercised.push_back(image);
    }
  }
  lucent::info("crashbash-ledger",
               "run-end images: {} module load(s) MATCHED a tracked specification and reached the "
               "publication boundary; {} published an authenticated generation; {} refused authentication; "
               "and the player published {} resident executable image(s). {} distinct tracked image(s) were "
               "exercised by this run",
               offered,
               published,
               refused,
               residentPublished,
               exercised.size());
  for (const ImagePublication &publication : facts_.publications) {
    lucent::info("crashbash-ledger",
                 "run-end image: {} -> '{}' generation {} over physical [0x{:06X},0x{:06X}) = {} sector(s), "
                 "{} byte(s), {} content witness(es); {} authenticated image generation(s) were active "
                 "before it, and publishing it made the runtime issue {} invalidation request(s) to the "
                 "block cache and translate {} more block(s)",
                 publication.logicalImage,
                 publication.name,
                 publication.generation,
                 publication.physicalBegin,
                 publication.physicalEnd,
                 publication.sectors,
                 publication.bytes,
                 publication.contentWitnesses,
                 publication.residenciesBefore,
                 publication.invalidations,
                 publication.blocksTranslated);
  }
  for (const ModuleOffer &offer : facts_.offers) {
    lucent::info("crashbash-ledger",
                 "run-end module offer: {} sector(s) from LBA {} to 0x{:08X} matched the tracked {} "
                 "specification",
                 offer.sectors,
                 offer.discLba,
                 offer.destination,
                 offer.logicalImage);
  }
  for (const std::string &refusal : facts_.refusals) {
    lucent::error("crashbash-ledger", "run-end module refusal: {}", refusal);
  }
  for (const std::string &refusal : facts_.residentRefusals) {
    lucent::error("crashbash-ledger", "run-end resident refusal: {}", refusal);
  }
  // A matched load that was neither published nor refused would be code that loaded guest bytes and
  // left them without an authenticated identity — the exact state the executor refuses as a typed
  // fault, discovered here instead of as a fault three hundred frames later.
  if (published + refused != offered) {
    lucent::error("crashbash-ledger",
                  "run-end images: {} module load(s) matched a tracked specification but {} were published "
                  "and {} were refused — {} load(s) reached guest RAM with NO authenticated image identity",
                  offered,
                  published,
                  refused,
                  offered > published + refused ? offered - published - refused : 0u);
  }
}

void RunLedger::reportNativeOwnership() const {
  lucent::info("crashbash-ledger",
               "run-end native owners: {} override(s) registered by this title, {} authenticated image "
               "generation(s) bound, {} original-body call(s) made through the scoped dynarec operation",
               facts_.registrations,
               facts_.boundImages,
               facts_.originalCalls);
  for (const std::string &site : facts_.originalCallSites) {
    lucent::info("crashbash-ledger", "run-end original call: {}", site);
  }
  for (const std::string &fault : facts_.faults) {
    lucent::error("crashbash-ledger", "run-end guest fault: {}", fault);
  }
  if (facts_.faultsDropped > 0u) {
    lucent::error("crashbash-ledger",
                  "run-end guest fault: {} further fault site(s) were observed and not listed — the fault "
                  "list is capped at 8, so this is not the whole set",
                  facts_.faultsDropped);
  }
}

void RunLedger::report(RunOutcome outcome, std::string_view why) {
  if (reported_) {
    lucent::error("crashbash-ledger",
                  "run-end ({}: {}) requested a SECOND time for one armed ledger; the first report already "
                  "named this run's outcome and two run-end ledgers are two denominators",
                  outcomeName(outcome),
                  why);
    return;
  }
  reported_ = true;
  lucent::info("crashbash-ledger", "run-end ({}: {}) — whole-run ledger", outcomeName(outcome), why);
  reportExecutionContext();
  reportImages();
  reportNativeOwnership();
  if (facts_.publications.empty()) {
    lucent::error("crashbash-ledger",
                  "run-end images: NO authenticated image was published and {} module load(s) matched a "
                  "tracked specification. Either nothing loaded, or the publication boundary was never "
                  "reached — the two are different failures and this line does not choose between them",
                  facts_.offers.size());
  }
}

void RunLedger::refuseRun(std::string_view why) {
  report(RunOutcome::Refused, why);
  std::abort();
}

void RunLedger::noteImagePublication(const ImagePublication &publication) {
  facts_.publications.push_back(publication);
}

void RunLedger::noteModuleOffer(const ModuleOffer &offer) {
  facts_.offers.push_back(offer);
}

void RunLedger::noteModuleRefusal(std::string_view logicalImage, std::string_view why) {
  facts_.refusals.emplace_back(std::string(logicalImage) + ": " + std::string(why));
}

void RunLedger::noteResidentRefusal(std::string_view why) {
  facts_.residentRefusals.emplace_back(why);
}

void RunLedger::noteOriginalCall(std::string_view logicalImage, std::uint32_t address) {
  ++facts_.originalCalls;
  std::string site(logicalImage);
  site += ' ';
  site += hex32(address);
  if (std::find(facts_.originalCallSites.begin(), facts_.originalCallSites.end(), site) ==
      facts_.originalCallSites.end()) {
    facts_.originalCallSites.push_back(std::move(site));
  }
}

void RunLedger::noteInstalledOverrides(std::size_t registrations, std::size_t boundImages) {
  facts_.registrations = registrations;
  facts_.boundImages = boundImages;
}

void RunLedger::noteGuestFault(std::string_view owner, std::uint32_t guestPc, std::string_view detail) {
  if (detail.find("Lightrec fallback refused") != std::string_view::npos) {
    ++facts_.fallbackRefusalFaults;
  }
  // Bounded, because a fault that does not stop the run would otherwise print without limit. The
  // dropped count is reported beside the kept ones, so a truncated fault list cannot read as a
  // complete one.
  constexpr std::size_t kMaxFaults = 8u;
  if (facts_.faults.size() >= kMaxFaults) {
    ++facts_.faultsDropped;
    return;
  }
  std::string text(owner);
  text += " ended at guest PC ";
  text += hex32(guestPc);
  text += ": ";
  text += detail;
  facts_.faults.push_back(std::move(text));
}

} // namespace crashbash::diagnostics