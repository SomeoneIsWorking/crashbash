// The whole-run ledger: what a run reports when it finishes, and what it reports when it refuses.
//
// The report is the only place this product states its translated, invalidated, fallback and image
// numbers, so the cases here are about the report being PRESENT and being HONEST rather than about
// any number being large: a run that executed nothing must say so loudly, a matched load that was
// neither published nor refused must be reported as the identity gap it is, and a run that refuses
// must emit the ledger before the process ends.

#include "boot_image.h"
#include "boot_image_identity.h"
#include "core.h"
#include "crashbash_guest.h"
#include "executable_identity.h"
#include "game.h"
#include "guest_execution.h"
#include "resident_image.h"
#include "run_ledger.h"
#include "testutil.h"
#include "title_adapter.h"

#include <algorithm>
#include <fstream>
#include <lucent/log.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

crashbash::TitleAdapter runtime;
std::vector<std::uint8_t> bootBytes;
std::vector<std::string> captured;

std::unique_ptr<Game> makeGame() {
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  runtime.registerOverrides(*game);
  return game;
}

// Capture the ledger's own channel instead of scraping a whole process log, so a case can assert
// that a line was printed AT ALL — a report that says nothing must fail, not pass quietly.
void captureLedgerOutput() {
  captured.clear();
  lucent::set_sink([](lucent::Level, std::string_view line) {
    captured.emplace_back(line);
  });
}

void releaseLedgerOutput() {
  lucent::set_sink(nullptr);
}

bool printed(std::string_view needle) {
  return std::any_of(captured.begin(), captured.end(), [needle](const std::string &line) {
    return line.find(needle) != std::string::npos;
  });
}

std::size_t linesContaining(std::string_view needle) {
  return static_cast<std::size_t>(std::count_if(captured.begin(), captured.end(), [needle](const std::string &line) {
    return line.find(needle) != std::string::npos;
  }));
}

// A case's ledger reports when it leaves scope, and `CHECK` returns from the case on the first
// failure, so the destructor is what keeps one case's unreported ledger from printing its own
// teardown line into the NEXT case's captured output. The ledger is per-instance, so there is no
// process state left behind for a later case to trip over — that is the point of the ownership
// change this file's cases exist to exercise.
struct LedgerClosed {
  ~LedgerClosed() {
    ledger.close("case teardown");
  }
  crashbash::diagnostics::RunLedger &ledger;
};

// Build a Game and the ledger its context owns, in one step: the ledger IS that Core's ledger, so a
// case can never measure a Core with another Core's census.
std::unique_ptr<Game> makeGameWithLedger(crashbash::diagnostics::RunLedger *&ledger) {
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  runtime.registerOverrides(*game);
  ledger = &runtime.runLedger(game->core);
  return game;
}

// Each Core's ledger reports when it is destroyed, so a case that never calls report() or close()
// still produces its ledger exactly once — that is the RAII half of the ownership change and the
// reason the product needs no "did I remember to close" discipline.
void test_a_ledger_that_is_never_closed_reports_when_it_is_destroyed() {
  auto game = makeGame();
  captureLedgerOutput();
  {
    crashbash::diagnostics::RunLedger owned(game->core);
    CHECK(!owned.reported());
  }
  releaseLedgerOutput();
  CHECK_EQ(linesContaining("whole-run ledger"), 1u);
  CHECK(printed("the Core was destroyed with this run unreported"));
}

// Two Cores have two ledgers, and neither can see the other's facts. Arming twice used to be a
// runtime error; it is now unrepresentable, and THIS is the case that would have caught a shared
// census reintroduced as a member of something larger.
void test_two_cores_own_two_independent_ledgers() {
  crashbash::diagnostics::RunLedger *firstLedger = nullptr;
  crashbash::diagnostics::RunLedger *secondLedger = nullptr;
  auto first = makeGameWithLedger(firstLedger);
  auto second = makeGameWithLedger(secondLedger);
  CHECK(firstLedger != secondLedger);
  firstLedger->noteResidentRefusal("the first run's own refusal");
  CHECK_EQ(firstLedger->facts().residentRefusals.size(), 1u);
  CHECK_EQ(secondLedger->facts().residentRefusals.size(), 0u);
  // And the refusals are reported by the ledger that recorded them.
  captureLedgerOutput();
  firstLedger->report(crashbash::diagnostics::RunOutcome::Refused, "first run only");
  releaseLedgerOutput();
  CHECK_EQ(linesContaining("the first run's own refusal"), 1u);
  CHECK(!secondLedger->reported());
}

// A run that executed nothing is the case a ledger exists to catch, and it is the one that reads
// most like success: every counter is a real zero. It must be loud, and it must be loud about the
// fact that the zeros measure nothing.
void test_a_run_that_executed_nothing_says_it_measured_nothing() {
  crashbash::diagnostics::RunLedger *ledger = nullptr;
  auto game = makeGameWithLedger(ledger);
  const LedgerClosed closeAtExit{*ledger};
  captureLedgerOutput();
  ledger->report(crashbash::diagnostics::RunOutcome::Completed, "ledger test");
  releaseLedgerOutput();
  CHECK(ledger->reported());
  CHECK(printed("whole-run ledger"));
  CHECK(printed("executed 0 guest instruction(s)"));
  CHECK(printed("measured NOTHING about dynarec execution"));
  // This owner must NOT reprint the dynarec's numbers: psxport's run-end guest line already carries
  // every reason by name, and a second copy in a second format is two answers to one question. These
  // assertions are the discriminator — they fail the moment a counter is re-derived here.
  CHECK(!printed("compilation_failed=0"));
  CHECK(!printed("run-end executor:"));
  CHECK(!printed("run-end cache:"));
  CHECK(!printed("run-end bounded exits:"));
  CHECK(!printed("run-end interpreter fallback:"));
  CHECK(printed("NO authenticated image was published"));
}

// The second report for one armed run is refused, because two run-end ledgers for one run are two
// denominators for the same measurement.
void test_one_ledger_reports_once() {
  crashbash::diagnostics::RunLedger *ledger = nullptr;
  auto game = makeGameWithLedger(ledger);
  const LedgerClosed closeAtExit{*ledger};
  captureLedgerOutput();
  ledger->report(crashbash::diagnostics::RunOutcome::Refused, "first");
  ledger->report(crashbash::diagnostics::RunOutcome::Completed, "second");
  releaseLedgerOutput();
  CHECK_EQ(linesContaining("whole-run ledger"), 1u);
  CHECK(printed("REFUSED"));
  CHECK(printed("requested a SECOND time"));
  CHECK(ledger->reported());
}

// A run ending by refusal emits the whole ledger, not a one-line apology: the numbers a refused run
// produced are the ones nobody otherwise has.
void test_a_refused_run_still_reports_the_ledger() {
  crashbash::diagnostics::RunLedger *ledger = nullptr;
  auto game = makeGameWithLedger(ledger);
  const LedgerClosed closeAtExit{*ledger};
  ledger->noteOriginalCall("menu", 0x800B5244u);
  ledger->noteOriginalCall("menu", 0x800B5244u);
  ledger->noteGuestFault("Crash Bash process update",
                         0x80018AA0u,
                         "Lightrec fallback refused before interpreter execution: "
                         "reason=jit_compile_failure, admitted_blocks=1, limit=1");
  captureLedgerOutput();
  ledger->report(crashbash::diagnostics::RunOutcome::Refused, "a guest loop");
  releaseLedgerOutput();
  CHECK(printed("REFUSED: a guest loop"));
  CHECK(printed("run-end guest fault: Crash Bash process update ended at guest PC 0x80018AA0"));
  // The runtime's own fault text is the only place a fallback reason reaches a port, so the refusal
  // census counts exactly that and nothing else.
  CHECK(printed("1 refusal site(s) were seen"));
  CHECK_EQ(ledger->facts().fallbackRefusalFaults, 1u);
  // Two calls at one site are two calls and one site.
  CHECK_EQ(ledger->facts().originalCalls, 2u);
  CHECK_EQ(ledger->facts().originalCallSites.size(), 1u);
  CHECK(printed("2 original-body call(s)"));
  CHECK(printed("original call: menu 0x800B5244"));
}

// A fault that is NOT a fallback refusal must not be counted as one. Both classes, in one run, so
// the census is a discriminator rather than a counter that always agrees.
void test_only_the_runtimes_own_fallback_text_counts_as_a_fallback_site() {
  crashbash::diagnostics::RunLedger *ledger = nullptr;
  auto game = makeGameWithLedger(ledger);
  const LedgerClosed closeAtExit{*ledger};
  ledger->noteGuestFault("Crash Bash guest call", 0x800B32B4u, "ambiguous code-image identity");
  ledger->noteGuestFault("Crash Bash guest call", 0x800C3434u, "crash");
  captureLedgerOutput();
  ledger->report(crashbash::diagnostics::RunOutcome::Refused, "image identity");
  releaseLedgerOutput();
  CHECK_EQ(ledger->facts().fallbackRefusalFaults, 0u);
  CHECK(printed("0 refusal site(s) were seen"));
  CHECK(printed("ambiguous code-image identity"));
  CHECK_EQ(ledger->facts().faults.size(), 2u);
}

// The unpublished-load check has to be able to FIRE, or it is decoration. Offered and neither
// published nor refused is exactly the state the executor later refuses as a typed fault, so the
// report names it as the identity gap it is.
void test_a_matched_load_left_unpublished_is_reported() {
  crashbash::diagnostics::RunLedger *ledger = nullptr;
  auto game = makeGameWithLedger(ledger);
  const LedgerClosed closeAtExit{*ledger};
  crashbash::diagnostics::ModuleOffer offer{"crashbash-usa-boot", 35799u, 0x80010000u, 189u};
  ledger->noteModuleOffer(offer);
  captureLedgerOutput();
  ledger->report(crashbash::diagnostics::RunOutcome::Completed, "unpublished load");
  releaseLedgerOutput();
  CHECK(printed("1 module load(s) MATCHED a tracked specification"));
  CHECK(printed("1 load(s) reached guest RAM with NO authenticated image identity"));
}

// The same ledger with the load accounted for: published on one side, refused on the other, and
// neither produces the identity gap. This is the positive control for the line above.
void test_published_and_refused_loads_both_account_for_their_offer() {
  crashbash::diagnostics::RunLedger *ledger = nullptr;
  auto game = makeGameWithLedger(ledger);
  const LedgerClosed closeAtExit{*ledger};
  ledger->noteModuleOffer({"crashbash-usa-boot", 35799u, 0x80010000u, 189u});
  ledger->noteModuleRefusal("crashbash-usa-boot", "guest RAM did not hold the authenticated bytes");
  ledger->noteModuleOffer({"crashbash-usa-menu", 28178u, 0x800B32B4u, 16u});
  crashbash::diagnostics::ImagePublication publication{};
  publication.logicalImage = "crashbash-usa-menu";
  publication.name = "crashbash-usa-menu";
  publication.generation = 4u;
  publication.physicalBegin = 0x000B32B4u;
  publication.physicalEnd = 0x000B3AB4u;
  publication.sectors = 16u;
  publication.bytes = 32768u;
  publication.contentWitnesses = 2;
  publication.residenciesBefore = 1u;
  publication.fromPublicationBoundary = true;
  publication.invalidations = 3u;
  ledger->noteImagePublication(publication);
  captureLedgerOutput();
  ledger->report(crashbash::diagnostics::RunOutcome::Completed, "accounted loads");
  releaseLedgerOutput();
  CHECK(!printed("NO authenticated image identity"));
  CHECK(printed("2 module load(s) MATCHED"));
  CHECK(printed("1 published an authenticated generation"));
  CHECK(printed("1 refused authentication"));
  CHECK(printed("player published 0 resident executable image(s)"));
  CHECK(printed("2 distinct tracked image(s) were exercised"));
  CHECK(printed("generation 4 over physical [0x0B32B4,0x0B3AB4)"));
  CHECK(printed("issue 3 invalidation request(s)"));
  CHECK(printed("run-end module refusal: crashbash-usa-boot"));
}

void test_authenticated_publication_feeds_the_ledger() {
  crashbash::diagnostics::RunLedger *ledger = nullptr;
  auto game = makeGameWithLedger(ledger);
  Core &core = game->core;
  CHECK_EQ(bootBytes.size(), crashbash::boot_image::kPayloadBytes);
  const LedgerClosed closeAtExit{*ledger};
  constexpr std::uint32_t physical = crashbash::boot_image::kLoadAddress & 0x1fffffffu;
  std::copy(bootBytes.begin(), bootBytes.end(), core.ram + physical);
  CHECK_EQ(crashbash::completeBootImageRead(core,
                                            crashbash::boot_image::kDiscLba,
                                            crashbash::boot_image::kLoadAddress,
                                            crashbash::boot_image::kSectorCount),
           crashbash::BootImageReadResult::Published);
  CHECK_EQ(ledger->facts().offers.size(), 1u);
  CHECK_EQ(ledger->facts().publications.size(), 1u);
  CHECK(ledger->facts().publications[0].generation > 0u);
  CHECK_EQ(ledger->facts().publications[0].sectors, crashbash::boot_image::kSectorCount);
  CHECK_EQ(ledger->facts().publications[0].bytes, crashbash::boot_image::kPayloadBytes);
  // A corrupt reload is offered, refused, and leaves the previous generation's fragments in place —
  // so the ledger's offer census and its publication census can differ without either being wrong.
  auto corrupt = bootBytes;
  corrupt.back() ^= 1u;
  std::copy(corrupt.begin(), corrupt.end(), core.ram + physical);
  CHECK_EQ(crashbash::completeBootImageRead(core,
                                            crashbash::boot_image::kDiscLba,
                                            crashbash::boot_image::kLoadAddress,
                                            crashbash::boot_image::kSectorCount),
           crashbash::BootImageReadResult::Rejected);
  CHECK_EQ(ledger->facts().offers.size(), 2u);
  CHECK_EQ(ledger->facts().publications.size(), 1u);
  CHECK_EQ(ledger->facts().refusals.size(), 1u);
  captureLedgerOutput();
  ledger->report(crashbash::diagnostics::RunOutcome::Completed, "boot published");
  releaseLedgerOutput();
  CHECK(printed("2 module load(s) MATCHED"));
  CHECK(printed("1 published an authenticated generation"));
  CHECK(!printed("NO authenticated image identity"));
}

// The player's own executable load is a refusal too, and it is NOT a module offer: nothing matched
// a tracked specification and nothing reached guest RAM. A run that stops here must still report, and
// must report the refusal rather than an image census that reads as "nothing was offered".
void test_a_refused_executable_still_reports_and_names_the_refusal() {
  crashbash::diagnostics::RunLedger *ledger = nullptr;
  auto game = makeGameWithLedger(ledger);
  const LedgerClosed closeAtExit{*ledger};
  std::vector<std::uint8_t> wrong(64u, 0x5au);
  CHECK(!crashbash::loadResidentImage(game->core, wrong, *ledger));
  CHECK_EQ(ledger->facts().residentRefusals.size(), 1u);
  // The right size with the wrong bytes is a DIFFERENT refusal, and it must be its own line: a
  // census that counted both as one "refused" would hide which witness rejected the image.
  std::vector<std::uint8_t> wrongBytes(crashbash::identity::kExecutableBytes, 0x5au);
  CHECK(!crashbash::loadResidentImage(game->core, wrongBytes, *ledger));
  CHECK_EQ(ledger->facts().residentRefusals.size(), 2u);
  CHECK_EQ(ledger->facts().offers.size(), 0u);
  CHECK_EQ(ledger->facts().publications.size(), 0u);
  captureLedgerOutput();
  ledger->report(crashbash::diagnostics::RunOutcome::Completed, "the executable failed authentication");
  releaseLedgerOutput();
  CHECK(printed("run-end resident refusal:"));
  CHECK(printed("executable SHA-256 does not match"));
  // Zero module offers and one resident refusal must NOT be reported as an unpublished module load:
  // that would be an identity gap for bytes that never entered RAM.
  CHECK(!printed("NO authenticated image identity"));
  CHECK(printed("measured NOTHING about dynarec execution"));
}

} // namespace

int main(int argc, char **argv) {
  RUN(a_run_that_executed_nothing_says_it_measured_nothing);
  RUN(one_ledger_reports_once);
  RUN(a_ledger_that_is_never_closed_reports_when_it_is_destroyed);
  RUN(two_cores_own_two_independent_ledgers);
  RUN(a_refused_run_still_reports_the_ledger);
  RUN(only_the_runtimes_own_fallback_text_counts_as_a_fallback_site);
  RUN(a_matched_load_left_unpublished_is_reported);
  RUN(published_and_refused_loads_both_account_for_their_offer);
  RUN(a_refused_executable_still_reports_and_names_the_refusal);
  if (argc == 2) {
    std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
    if (!input || input.tellg() != static_cast<std::streamoff>(crashbash::boot_image::kPayloadBytes)) {
      return 2;
    }
    bootBytes.resize(static_cast<std::size_t>(input.tellg()));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char *>(bootBytes.data()), static_cast<std::streamsize>(bootBytes.size()))) {
      return 2;
    }
    RUN(authenticated_publication_feeds_the_ledger);
  } else if (argc != 1) {
    return 2;
  }
  return pt_summary();
}
