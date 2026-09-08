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
  std::optional<psx::cpu::NativeKey> activeKey(GuestImage image, std::uint32_t address) const;
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

// Enter ordinary guest code through the runtime dispatcher. Callers establish the measured guest
// ABI state (including r31 and instruction timing) before entering this boundary.
void dispatchGuest(Core &core, std::uint32_t address);

// Execute the authenticated guest body for the currently active override through Lightrec while
// suppressing only that override. This is the sole replacement for generated "super" bodies.
void callOriginal(Core &core, GuestImage image, std::uint32_t address);

} // namespace crashbash::runtime
