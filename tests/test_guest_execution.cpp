#include "guest_execution.h"

#include "game.h"
#include "game_runtime.h"
#include "lightrec_executor.h"
#include "native_dispatch.h"
#include "testutil.h"

#include <memory>
#include <optional>
#include <stdexcept>

namespace {

using crashbash::runtime::GuestExecution;
using crashbash::runtime::GuestImage;
constexpr std::uint32_t kEntry = 0x80010000u;
constexpr std::uint32_t kReturn = 0x80010100u;
constexpr GuestAddressRange kRange{0x10000u, 0x10200u};

class Runtime final : public GameRuntime {
public:
  void *createContext(Core &core) override {
    return new GuestExecution(core);
  }
  void destroyContext(void *context) override {
    delete static_cast<GuestExecution *>(context);
  }
  void registerOverrides(Game &) override {}
  void bootInit(Core &) override {}
  RenderCapabilities renderCapabilities() const override {
    return RenderCapabilities::direct();
  }
  bool guestVramIsPicture(const Game &) const override {
    return false;
  }
};

std::unique_ptr<Game> makeGame(Runtime &runtime) {
  psxport_install_game(runtime);
  return std::make_unique<Game>();
}

GuestExecution &context(Core &core) {
  return *static_cast<GuestExecution *>(core.gameCtx);
}

void nativeValue(Core *core) {
  core->r[2] = 42u;
}

void nativeOriginal(Core *core) {
  crashbash::runtime::callOriginal(*core, GuestImage::Menu, kEntry);
  core->r[2] += 10u;
}

void test_pending_registration_reload_and_wrong_image_refusal() {
  Runtime runtime;
  auto game = makeGame(runtime);
  Core &core = game->core;
  GuestExecution &execution = context(core);
  crashbash::runtime::registerNativeOverride(core, GuestImage::Menu, kEntry, "menu-value", nativeValue);
  CHECK(!execution.activeKey(GuestImage::Menu, kEntry));
  const auto first = core.imageCatalog().activate("menu", kRange, 1u);
  execution.bindAuthenticatedImage(GuestImage::Menu, first, kRange);
  CHECK(core.nativeDispatcher().isInstalled({first, kEntry}));
  core.r[31] = kReturn;
  crashbash::runtime::dispatchGuest(core, kEntry);
  CHECK_EQ(core.r[2], 42u);
  const auto replacement = core.imageCatalog().activate("polar", kRange, 2u);
  CHECK(!execution.activeKey(GuestImage::Menu, kEntry));
  CHECK_EQ(execution.original(GuestImage::Menu, kEntry, psx::cpu::ExecutionBudget::fromCycles(100)).reason,
           psx::cpu::ExecutionExitReason::Fault);
  execution.bindAuthenticatedImage(GuestImage::Dat22510, replacement, kRange);
  CHECK(!execution.activeKey(GuestImage::Menu, kEntry));
  CHECK(!core.nativeDispatcher().isInstalled({replacement, kEntry}));
  const auto reload = core.imageCatalog().activate("menu", kRange, 1u);
  execution.bindAuthenticatedImage(GuestImage::Menu, reload, kRange);
  CHECK(!core.nativeDispatcher().isInstalled({first, kEntry}));
  CHECK(core.nativeDispatcher().isInstalled({reload, kEntry}));
  execution.unbindImage(GuestImage::Menu);
  CHECK(!core.nativeDispatcher().isInstalled({reload, kEntry}));
  CHECK(!execution.activeKey(GuestImage::Menu, kEntry));
}

void test_original_uses_dynarec_and_restores_interception() {
  Runtime runtime;
  auto game = makeGame(runtime);
  Core &core = game->core;
  GuestExecution &execution = context(core);
  const auto image = core.imageCatalog().activate("menu", kRange, 1u);
  core.mem_w32(kEntry, 0x24020007u);      // addiu v0, zero, 7
  core.mem_w32(kEntry + 4u, 0x03e00008u); // jr ra
  core.mem_w32(kEntry + 8u, 0u);          // delay-slot nop
  execution.bindAuthenticatedImage(GuestImage::Menu, image, kRange);
  crashbash::runtime::registerNativeOverride(core, GuestImage::Menu, kEntry, "menu-original", nativeOriginal);
  core.r[31] = kReturn;
  core.r[29] = 0x801ff000u;
  crashbash::runtime::dispatchGuest(core, kEntry);
  CHECK_EQ(core.r[2], 17u);
  CHECK_EQ(core.r[29], 0x801ff000u);
  CHECK_EQ(core.r[31], kReturn);
  CHECK(core.nativeDispatcher().intercepts({image, kEntry}));
  CHECK(core.lightrecExecutor().counters().executedBlocks > 0u);
  CHECK(core.lightrecExecutor().counters().executedInstructions > 0u);
  CHECK_EQ(core.lightrecExecutor().counters().fallback.calls, 0u);
  core.mem_w32(kEntry, 0x24020009u); // same generation, changed code must invalidate the compiled block
  crashbash::runtime::dispatchGuest(core, kEntry);
  CHECK_EQ(core.r[2], 19u);
  CHECK_EQ(execution.original(GuestImage::Boot, kEntry, psx::cpu::ExecutionBudget::fromCycles(100)).reason,
           psx::cpu::ExecutionExitReason::Fault);
}

void test_original_budget_exit_exposes_live_loop_state() {
  Runtime runtime;
  auto game = makeGame(runtime);
  Core &core = game->core;
  GuestExecution &execution = context(core);
  const auto image = core.imageCatalog().activate("menu", kRange, 1u);
  core.mem_w32(kEntry, 0x2508ffffu);       // addiu t0, t0, -1
  core.mem_w32(kEntry + 4u, 0x1500fffeu);  // bne t0, zero, kEntry
  core.mem_w32(kEntry + 8u, 0u);           // delay-slot nop
  core.mem_w32(kEntry + 12u, 0x03e00008u); // jr ra
  core.mem_w32(kEntry + 16u, 0u);          // delay-slot nop
  execution.bindAuthenticatedImage(GuestImage::Menu, image, kRange);
  crashbash::runtime::registerNativeOverride(core, GuestImage::Menu, kEntry, "menu-loop", nativeOriginal);
  core.r[31] = kReturn;
  core.r[8] = 1000u;
  const auto bounded = execution.original(GuestImage::Menu, kEntry, psx::cpu::ExecutionBudget::fromCycles(100u));
  CHECK_EQ(bounded.reason, psx::cpu::ExecutionExitReason::BudgetExhausted);
  CHECK(core.r[8] < 1000u);
  CHECK(core.r[8] > 0u);
  core.r[8] = 3u;
  const auto completed = execution.original(GuestImage::Menu, kEntry, psx::cpu::ExecutionBudget::fromCycles(100u));
  CHECK_EQ(completed.reason, psx::cpu::ExecutionExitReason::GuestReturn);
  CHECK_EQ(core.r[8], 0u);
}

// A host turn boundary the harness supplies directly, so the resume path is exercised without
// depending on how many cycles a host happens to retire inside one translated segment. The pc is a
// LIVE mid-function address and the cycle count is non-zero, which is exactly the shape of a real
// turn the fence accepts; a zero-cycle or zero-PC exit is the one it refuses.
psx::cpu::ExecutionResult syntheticTurn(std::uint32_t resumePc) {
  return {psx::cpu::ExecutionExitReason::BudgetExhausted, resumePc, 500u, "synthetic turn boundary"};
}

// A bounded guest body: a short counted loop, then r[2] = 42 and a return. Deliberately free of nested
// calls, because a nested `jal` overwrites r[31] and this fixture's whole claim is that the resume
// ends at the call's OWN return address.
//
// NOT COVERED HERE, and deliberately: that a resumed ORIGINAL suppresses its own override while a
// plain resume does not. Whether the executor stops at an override address is a translated-block
// decision, so that difference is not observable from a fixture this size; psxport records the same
// gap for the primitive (native_dispatch.cpp, "COVERAGE"). What is observable is that both resumes
// carry a cut call to its return address, which is what was broken.
void test_bounded_resume_carries_a_long_call_to_its_return() {
  Runtime runtime;
  auto game = makeGame(runtime);
  Core &core = game->core;
  GuestExecution &execution = context(core);
  const auto image = core.imageCatalog().activate("menu", kRange, 1u);
  constexpr std::uint32_t kBody = 0x80010030u;
  core.mem_w32(kBody, 0x24080005u);       // addiu t0, zero, 5
  core.mem_w32(kBody + 4u, 0x2508ffffu);  // addiu t0, t0, -1
  core.mem_w32(kBody + 8u, 0x1500fffeu);  // bne t0, zero, kBody + 4u
  core.mem_w32(kBody + 12u, 0u);          // delay-slot nop
  core.mem_w32(kBody + 16u, 0x2402002au); // addiu v0, zero, 42
  core.mem_w32(kBody + 20u, 0x03e00008u); // jr ra
  core.mem_w32(kBody + 24u, 0u);          // delay-slot nop
  execution.bindAuthenticatedImage(GuestImage::Menu, image, kRange);
  core.r[31] = kReturn;
  core.r[29] = 0x801ff000u;
  // A latched host/interrupt request is serviced at the first block boundary and execution continues
  // from the Core's own pc, which is not that boundary's pc. The product services pending work before
  // every guest call; this harness has no frame driver to do it, so the fixture does it explicitly.
  core.pending_work = 0;
  core.r[2] = 0u;
  core.r[8] = 0u;

  // A turn that ended budget-exhausted at a live mid-function pc is resumed to the call's return
  // address, and the resumed work actually runs: the loop's counter reaches its end value.
  const auto plain = crashbash::runtime::runGuestCallToReturn(
      core, kBody, kReturn, "test plain resumed guest call", std::nullopt, syntheticTurn(kBody));
  CHECK(plain.returned());
  CHECK_EQ(core.r[2], 42u);
  CHECK_EQ(core.r[8], 0u);
  CHECK_EQ(core.r[31], kReturn);
  CHECK_EQ(core.r[29], 0x801ff000u);
  CHECK(plain.cycles > 0u);

  // The same call resumed as the ORIGINAL of a key: that is the other framework entry point, and it
  // re-establishes the suppression and caller scopes the resume needs.
  crashbash::runtime::registerNativeOverride(core, GuestImage::Menu, kEntry, "menu-value", nativeValue);
  const psx::cpu::NativeKey key{image, kEntry};
  core.pending_work = 0;
  core.r[2] = 0u;
  core.r[8] = 0u;
  const auto original = crashbash::runtime::runGuestCallToReturn(
      core, kBody, kReturn, "test resumed original", key, syntheticTurn(kBody));
  CHECK(original.returned());
  CHECK_EQ(core.r[2], 42u);
  CHECK_EQ(core.r[8], 0u);
  CHECK_EQ(core.r[31], kReturn);
  // The suppression scope is released when the resumed original returns, so the dispatcher is back to
  // intercepting that key rather than left permanently suppressed — a leaked scope would silently
  // disable every native override from here on.
  CHECK(core.nativeDispatcher().intercepts({image, kEntry}));
}

void test_invalid_replacement_preserves_binding_and_cores_are_isolated() {
  Runtime runtime;
  auto first = makeGame(runtime);
  auto second = makeGame(runtime);
  Core &core = first->core;
  auto &execution = context(core);
  const auto image = core.imageCatalog().activate("menu", kRange, 1u);
  execution.registerOverride(GuestImage::Menu, kEntry, "value", nativeValue);
  execution.bindAuthenticatedImage(GuestImage::Menu, image, kRange);
  bool refused = false;
  try {
    execution.bindAuthenticatedImage(GuestImage::Menu, {image.id, image.generation + 1u}, kRange);
  } catch (const std::invalid_argument &) {
    refused = true;
  }
  CHECK(refused);
  CHECK(core.nativeDispatcher().isInstalled({image, kEntry}));
  CHECK(execution.activeKey(GuestImage::Menu, kEntry).has_value());
  CHECK(!context(second->core).activeKey(GuestImage::Menu, kEntry));
  CHECK(!second->core.nativeDispatcher().isInstalled({image, kEntry}));
  refused = false;
  try {
    execution.registerOverride(GuestImage::Menu, kEntry, "duplicate", nativeValue);
  } catch (const std::invalid_argument &) {
    refused = true;
  }
  CHECK(refused);
  refused = false;
  try {
    execution.registerOverride(GuestImage::Menu, kRange.end, "outside", nativeValue);
  } catch (const std::invalid_argument &) {
    refused = true;
  }
  CHECK(refused);
  core.imageCatalog().deactivate(image);
  CHECK(!execution.activeKey(GuestImage::Menu, kEntry));
}

} // namespace

int main() {
  RUN(pending_registration_reload_and_wrong_image_refusal);
  RUN(original_uses_dynarec_and_restores_interception);
  RUN(original_budget_exit_exposes_live_loop_state);
  RUN(bounded_resume_carries_a_long_call_to_its_return);
  RUN(invalid_replacement_preserves_binding_and_cores_are_isolated);
  return pt_summary();
}
