#pragma once

#include <cstdint>
#include <string_view>

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

// The single missing integration boundary while psxport connects its Lightrec backend and exposes
// loaded-image lifecycle binding. Implementations belong in a thin adapter over the shared per-Core
// API, never in generated title code or process-global state.
void registerNativeOverride(
    Core &core, GuestImage image, std::uint32_t address, std::string_view name, NativeOverride function);

// Enter ordinary guest code through the runtime dispatcher. Callers establish the measured guest
// ABI state (including r31 and instruction timing) before entering this boundary.
void dispatchGuest(Core &core, std::uint32_t address);

// Execute the authenticated guest body for the currently active override through Lightrec while
// suppressing only that override. This is the sole replacement for generated "super" bodies.
void callOriginal(Core &core, GuestImage image, std::uint32_t address);

} // namespace crashbash::runtime
