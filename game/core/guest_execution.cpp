#include "guest_execution.h"

#include "core.h"
#include "image_identity.h"
#include "lightrec_executor.h"

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

} // namespace

GuestExecution::GuestExecution(Core &core) : core_(core) {
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
  }
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
    const auto remaining = core_.imageCatalog().subtractRange(binding->identity, physicalRange);
    if (remaining == 0u) {
      removeRegistrations(*binding);
      binding = bindings_.erase(binding);
      continue;
    }
    for (const auto &registration : registrations_) {
      if (registration.image == binding->image && physicalRange.containsPhysical(registration.address)) {
        core_.nativeDispatcher().remove({binding->identity, registration.address});
      }
    }
    ++binding;
  }
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

void registerNativeOverride(
    Core &core, GuestImage image, std::uint32_t address, std::string_view name, NativeOverride function) {
  execution(core).registerOverride(image, address, name, function);
}

void retireAuthenticatedImagesForWrite(Core &core, GuestAddressRange physicalRange) {
  execution(core).retireImagesOverlapping(physicalRange);
}

void dispatchGuest(Core &core, std::uint32_t address) {
  psx::cpu::dispatchGuestToReturn(core, address, psx::cpu::ExecutionBudget::currentTurn(core), "Crash Bash guest call");
}

void callOriginal(Core &core, GuestImage image, std::uint32_t address) {
  const auto result = execution(core).original(image, address, psx::cpu::ExecutionBudget::currentTurn(core));
  lucent::debug("crashbash-original",
                "target=0x{:08X} exit={} pc=0x{:08X} cycles={} r4=0x{:08X} r5=0x{:08X} r6=0x{:08X} "
                "r7=0x{:08X} r8=0x{:08X} r16=0x{:08X} r17=0x{:08X} r18=0x{:08X} ra=0x{:08X}",
                address,
                psx::cpu::executionExitName(result.reason),
                result.guestPc,
                result.cycles,
                core.r[4],
                core.r[5],
                core.r[6],
                core.r[7],
                core.r[8],
                core.r[16],
                core.r[17],
                core.r[18],
                core.r[31]);
  if (!psx::cpu::requireGuestReturn(result, "Crash Bash original call")) {
    std::abort();
  }
}

} // namespace crashbash::runtime
