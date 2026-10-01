#include "resident_image.h"

#include "core.h"
#include "executable_identity.h"
#include "run_ledger.h"

#include <lucent/content.h>

namespace crashbash {

psx::cpu::PsxExeLoadResult
loadResidentImage(Core &core, std::span<const std::uint8_t> bytes, diagnostics::RunLedger &ledger) {
  if (bytes.size() != identity::kExecutableBytes) {
    ledger.noteResidentRefusal("Crash Bash USA executable size does not match the authenticated manifest");
    return {std::nullopt, {}, "Crash Bash USA executable size does not match the authenticated manifest"};
  }
  if (lucent::content::sha256_hex(lucent::content::sha256(std::as_bytes(bytes))) != identity::kExecutableSha256) {
    ledger.noteResidentRefusal("Crash Bash USA executable SHA-256 does not match the authenticated manifest");
    return {std::nullopt, {}, "Crash Bash USA executable SHA-256 does not match the authenticated manifest"};
  }
  const psx::cpu::PsxExeLoadResult loaded = psx::cpu::loadPsxExeImage(core, bytes, "crashbash-usa-resident");
  if (!loaded) {
    ledger.noteResidentRefusal(loaded.detail);
    return loaded;
  }
  // The resident executable is a load like any nested module: it publishes a generation, and the run
  // ledger records it so a run that authenticated the executable but published no image for it is
  // visible as the mismatch it is. The published range is the framework's own authenticated code
  // interval, not the file: the pre-BSS text is what a translated block can belong to.
  diagnostics::ImagePublication publication{};
  publication.logicalImage = "resident";
  publication.name = "crashbash-usa-resident";
  publication.generation = loaded.identity->generation;
  publication.physicalBegin = loaded.image.physicalText.begin;
  publication.physicalEnd = loaded.image.physicalText.end;
  publication.sectors = 0u;
  publication.bytes = loaded.image.physicalText.end - loaded.image.physicalText.begin;
  publication.contentWitnesses = 1;
  publication.residenciesBefore = core.imageCatalog().activeCount();
  ledger.noteImagePublication(publication);
  return loaded;
}

} // namespace crashbash
