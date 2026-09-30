#include "scene_machine.h"

#include "core.h"
#include "crashbash_guest.h"
#include "guest_execution.h"

#include <cstdint>
#include <lucent/log.h>

namespace crashbash::diagnostics {
namespace {

constexpr std::uint32_t kSceneRecord = guest::kSceneTransition;
constexpr std::uint32_t kSceneClock = guest::kSceneTransitionClock;
// Words of the scene record `0x8001E610` reads, in its own order: {current, target, previous, flags}.
constexpr std::uint32_t kCurrentSlot = guest::kSceneCurrentSlot;
constexpr std::uint32_t kTargetSlot = guest::kSceneTargetSlot;
constexpr std::uint32_t kPreviousSlot = guest::kScenePreviousSlot;
constexpr std::uint32_t kFlagsSlot = guest::kSceneFlagsSlot;

const char *sceneKind(std::uint32_t owner) {
  return owner == kSceneRecord ? "scene" : (owner == kSceneClock ? "clock" : "other record");
}

void sceneRequestOwned(Core *core) {
  // The guest's arguments, read before the original runs. `0x8001E588` is called both as
  // request(scene, target, flags) and, by a scene that wants a fast hand-off, as arm(clock, step, 0)
  // — so the record being addressed is reported rather than assumed.
  const std::uint32_t record = core->r[4];
  const std::uint32_t target = core->r[5];
  const std::uint32_t flags = core->r[6];
  const std::uint32_t currentBefore = core->mem_r32(kSceneRecord + kCurrentSlot);
  const std::uint32_t targetBefore = core->mem_r32(kSceneRecord + kTargetSlot);
  const std::uint32_t ageBefore = core->mem_r32(kSceneClock + guest::kSceneClockAgeSlot);

  runtime::callOriginal(*core, runtime::GuestImage::Resident, guest::kSceneTransitionRequest);

  // What the machine actually did with the request. A request that leaves current unchanged is the
  // normal outcome while the transition clock ages toward its gate, so the two are reported apart:
  // "asked for X" and "entered X" are different claims and only the second is a transition.
  const std::uint32_t currentAfter = core->mem_r32(kSceneRecord + kCurrentSlot);
  const std::uint32_t targetAfter = core->mem_r32(kSceneRecord + kTargetSlot);
  const std::uint32_t ageAfter = core->mem_r32(kSceneClock + guest::kSceneClockAgeSlot);
  lucent::info("crashbash-scene",
               "guest requested {} 0x{:08X} -> 0x{:08X} request-flags=0x{:X}: current 0x{:08X}->0x{:08X} "
               "target 0x{:08X}->0x{:08X} previous=0x{:08X} record-flags=0x{:X} clock-age {}->{}",
               sceneKind(record),
               record,
               target,
               flags,
               currentBefore,
               currentAfter,
               targetBefore,
               targetAfter,
               core->mem_r32(kSceneRecord + kPreviousSlot),
               core->mem_r32(kSceneRecord + kFlagsSlot),
               ageBefore,
               ageAfter);
}

} // namespace

void registerSceneMachine(Core &core) {
  runtime::registerNativeOverride(core,
                                  runtime::GuestImage::Resident,
                                  guest::kSceneTransitionRequest,
                                  "CrashBashDiagnostics::sceneRequest",
                                  sceneRequestOwned);
}

} // namespace crashbash::diagnostics
