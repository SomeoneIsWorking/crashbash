#include "nested_module_image.h"

#include "dat22510_image_identity.h"
#include "dat28136_image_identity.h"
#include "dat28241_image_identity.h"
#include "dat28272_image_identity.h"
#include "dat28382_image_identity.h"
#include "guest_execution.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace crashbash {
namespace {

// One specification per tracked nested overlay, built from the CMake-derived identity header of the
// SAME manifest provisioning authenticates. There is no literal here that the manifest does not
// already carry, so a re-provisioned image cannot leave these stale.
constexpr std::array<AuthenticatedModuleSpec, 5> kNestedSpecs{
    AuthenticatedModuleSpec{runtime::GuestImage::Dat28136,
                            "crashbash-usa-dat28136",
                            dat28136_image::kDiscLba,
                            dat28136_image::kLoadAddress,
                            dat28136_image::kSectorCount,
                            dat28136_image::kPayloadBytes,
                            dat28136_image::kSha256,
                            dat28136_image::kEntryPointerOffset,
                            dat28136_image::kEntry},
    AuthenticatedModuleSpec{runtime::GuestImage::Dat28272,
                            "crashbash-usa-dat28272",
                            dat28272_image::kDiscLba,
                            dat28272_image::kLoadAddress,
                            dat28272_image::kSectorCount,
                            dat28272_image::kPayloadBytes,
                            dat28272_image::kSha256,
                            dat28272_image::kEntryPointerOffset,
                            dat28272_image::kEntry},
    AuthenticatedModuleSpec{runtime::GuestImage::Dat28241,
                            "crashbash-usa-dat28241",
                            dat28241_image::kDiscLba,
                            dat28241_image::kLoadAddress,
                            dat28241_image::kSectorCount,
                            dat28241_image::kPayloadBytes,
                            dat28241_image::kSha256,
                            dat28241_image::kEntryPointerOffset,
                            dat28241_image::kEntry},
    AuthenticatedModuleSpec{runtime::GuestImage::Dat28382,
                            "crashbash-usa-dat28382",
                            dat28382_image::kDiscLba,
                            dat28382_image::kLoadAddress,
                            dat28382_image::kSectorCount,
                            dat28382_image::kPayloadBytes,
                            dat28382_image::kSha256,
                            dat28382_image::kEntryPointerOffset,
                            dat28382_image::kEntry},
    AuthenticatedModuleSpec{runtime::GuestImage::Dat22510,
                            "crashbash-usa-dat22510",
                            dat22510_image::kDiscLba,
                            dat22510_image::kLoadAddress,
                            dat22510_image::kSectorCount,
                            dat22510_image::kPayloadBytes,
                            dat22510_image::kSha256,
                            dat22510_image::kEntryPointerOffset,
                            dat22510_image::kEntry},
};

static_assert(validModuleSpec(kNestedSpecs[0]));
static_assert(validModuleSpec(kNestedSpecs[1]));
static_assert(validModuleSpec(kNestedSpecs[2]));
static_assert(validModuleSpec(kNestedSpecs[3]));
static_assert(validModuleSpec(kNestedSpecs[4]));

// The nested slot is one address, so a specification that claimed a different one would be a second
// claim on the same bytes. Asserting it here means a manifest edit that moves a load address cannot
// pass unnoticed and quietly publish two owners of one range.
constexpr std::uint32_t kNestedLoadAddress = 0x800B32B4u;
static_assert(kNestedSpecs[0].loadAddress == kNestedLoadAddress);
static_assert(kNestedSpecs[1].loadAddress == kNestedLoadAddress);
static_assert(kNestedSpecs[2].loadAddress == kNestedLoadAddress);
static_assert(kNestedSpecs[3].loadAddress == kNestedLoadAddress);
static_assert(kNestedSpecs[4].loadAddress == kNestedLoadAddress);

// Two specifications with the same disc LBA would make a read match either, and the first to
// authenticate would win. Distinct LBAs are what make the match unambiguous.
constexpr bool distinctDiscLbas() {
  for (std::size_t a = 0; a < kNestedSpecs.size(); ++a) {
    for (std::size_t b = a + 1; b < kNestedSpecs.size(); ++b) {
      if (kNestedSpecs[a].discLba == kNestedSpecs[b].discLba) {
        return false;
      }
    }
  }
  return true;
}
static_assert(distinctDiscLbas());

} // namespace

std::span<const AuthenticatedModuleSpec> NestedModuleImage::specs() {
  return std::span<const AuthenticatedModuleSpec>(kNestedSpecs.data(), kNestedSpecs.size());
}

std::size_t NestedModuleImage::specCount() {
  return kNestedSpecs.size();
}

bool NestedModuleImage::isNestedModuleRead(std::uint32_t lba, std::uint32_t destination, std::uint32_t sectorCount) {
  for (const AuthenticatedModuleSpec &spec : kNestedSpecs) {
    if (isModuleImageRead(spec, lba, destination, sectorCount)) {
      return true;
    }
  }
  return false;
}

ModuleImageReadResult
NestedModuleImage::offer(Core &core, std::uint32_t lba, std::uint32_t destination, std::uint32_t sectorCount) {
  // The read is offered to EVERY specification and the results are combined, so "no spec matched"
  // and "a spec matched and the bytes were not it" stay different answers: the caller must fail a
  // read whose module identity did not authenticate, and must not fail the hundreds of asset reads
  // that belong to no module at all.
  bool anyMatched = false;
  ModuleImageReadResult published = ModuleImageReadResult::Unrelated;
  for (const AuthenticatedModuleSpec &spec : kNestedSpecs) {
    const ModuleImageReadResult result = completeModuleImageRead(core, spec, lba, destination, sectorCount);
    if (result != ModuleImageReadResult::Unrelated) {
      anyMatched = true;
    }
    if (result == ModuleImageReadResult::Rejected) {
      return result;
    }
    if (result == ModuleImageReadResult::Published) {
      published = result;
    }
  }
  return anyMatched ? published : ModuleImageReadResult::Unrelated;
}

} // namespace crashbash
