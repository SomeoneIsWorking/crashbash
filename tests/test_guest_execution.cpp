#include "guest_execution.h"

#include "game.h"
#include "game_runtime.h"
#include "lightrec_executor.h"
#include "testutil.h"

#include <memory>
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
  RUN(invalid_replacement_preserves_binding_and_cores_are_isolated);
  return pt_summary();
}
