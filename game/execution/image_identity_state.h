#pragma once

#include "guest_execution.h"
#include "machine_state.h"

#include <optional>
#include <string>
#include <vector>

namespace crashbash::runtime {

// Crash Bash's native state in a whole-machine save state: the authenticated image identities of the
// code resident in guest RAM.
//
// A restore replaces guest RAM wholesale, so without this section the catalog would keep describing
// whatever THIS session last loaded — a Polar Push state restored over a session holding MENU in the
// nested slot would run DAT22510's code under MENU's identity. The identities cannot be re-derived
// from the restored bytes either: a live module no longer hashes to its manifest digest, because its
// data lives inside its payload.
//
// `load` validates and stages the records before anything is restored; `restored` publishes them as
// fresh generations once the RAM they describe is in place (psx::state::NativeStatePort::restored).
class ImageIdentityState final : public psx::state::NativeStatePort {
public:
  explicit ImageIdentityState(GuestExecution &execution) : execution_(execution) {}

  [[nodiscard]] const char *sectionName() const override {
    return "crashbash.image-identity";
  }
  [[nodiscard]] std::uint32_t version() const override {
    return 1;
  }
  bool save(psx::state::BlobWriter &out, std::string &error) const override;
  bool load(psx::state::BlobReader &in, std::string &error) override;
  void restored(Core &core) override;

private:
  GuestExecution &execution_;
  std::optional<std::vector<BoundImageRecord>> staged_;
};

// The record list a state section carries, and its inverse. `readBoundImages` refuses, by `error`,
// anything `writeBoundImages` could not have produced from a live title: an unknown logical image, a
// range outside main RAM, surviving ranges that are empty, unsorted, overlapping or outside their
// published range, a logical image recorded twice, or no resident executable.
void writeBoundImages(psx::state::BlobWriter &out, const std::vector<BoundImageRecord> &records);
std::optional<std::vector<BoundImageRecord>> readBoundImages(psx::state::BlobReader &in, std::string &error);

} // namespace crashbash::runtime
