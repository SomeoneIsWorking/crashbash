#include "cd_file_read.h"

#include "boot_image.h"
#include "core.h"
#include "crashbash_guest.h"
#include "disc.h"
#include "game.h"
#include "guest_execution.h"
#include "menu_image.h"
#include "nested_module_image.h"

#include <array>
#include <cstdint>
#include <lucent/log.h>
#include <optional>

namespace crashbash {
namespace {

constexpr std::uint32_t kSectorBytes = 2048u;

} // namespace

void retireImagesForCdSectorWrite(Core &core, std::uint32_t destination) {
  if (const auto mapped = core.mappedMainRamRange(destination, kSectorBytes)) {
    runtime::retireAuthenticatedImagesForWrite(core, *mapped);
    return;
  }
  // The sector may cross a mirrored-RAM boundary. Use Core's actual mapping so an I/O or
  // scratchpad destination cannot accidentally retire a main-RAM image.
  std::optional<GuestAddressRange> run;
  const auto flush = [&] {
    if (run) {
      runtime::retireAuthenticatedImagesForWrite(core, *run);
      run.reset();
    }
  };
  for (std::uint32_t byte = 0; byte < kSectorBytes; ++byte) {
    const auto mapped = core.mappedMainRamRange(destination + byte, 1u);
    if (!mapped) {
      flush();
      continue;
    }
    const auto offset = mapped->begin;
    if (!run || run->end != offset) {
      flush();
      run = GuestAddressRange{offset, offset + 1u};
    } else {
      ++run->end;
    }
  }
  flush();
}

namespace {

void cdFileReadOwned(Core *core) {
  // Retail 0x80027790 starts an interrupt-driven 2048-byte-sector read, then returns its async
  // completion state. The shipping port has no asynchronous CD controller: complete the same
  // descriptor-relative interval from the real native disc before returning. Success is reported
  // only after every requested sector has been copied into guest RAM.
  const std::uint32_t descriptor = core->r[4];
  const std::uint32_t descriptorOffset = core->r[5];
  const std::uint32_t destination = core->r[6];
  const std::uint32_t sectorCount = core->r[7];
  const std::uint32_t lba = core->mem_r32(descriptor) + descriptorOffset + core->mem_r32(guest::kCdBaseLba);
  std::array<std::uint8_t, kSectorBytes> sector{};

  for (std::uint32_t index = 0; index < sectorCount; ++index) {
    if (!disc_read_sector(&core->game->disc, lba + index, sector.data())) {
      core->mem_w32(guest::kCdReadActive, 0u);
      core->r[2] = 0xFFFFFFFFu;
      lucent::error("crashbash-cd", "native file read failed at LBA {} ({}/{})", lba + index, index, sectorCount);
      return;
    }
    const std::uint32_t sectorDestination = destination + index * kSectorBytes;
    retireImagesForCdSectorWrite(*core, sectorDestination);
    for (std::uint32_t byte = 0; byte < kSectorBytes; ++byte) {
      core->mem_w8(sectorDestination + byte, sector[byte]);
    }
  }

  if (completeBootImageRead(*core, lba, destination, sectorCount) == BootImageReadResult::Rejected ||
      completeMenuImageRead(*core, lba, destination, sectorCount) == MenuImageReadResult::Rejected ||
      NestedModuleImage::offer(*core, lba, destination, sectorCount) == ModuleImageReadResult::Rejected) {
    core->mem_w32(guest::kCdReadActive, 0u);
    core->r[2] = 0xFFFFFFFFu;
    return;
  }

  core->mem_w32(guest::kCdReadActive, 0u);
  core->r[2] = 0u;
  lucent::debug("crashbash-cd",
                "native file read completed: {} sector(s) from LBA {} to 0x{:08X}",
                sectorCount,
                lba,
                destination);
}

constexpr std::uint32_t kLoadPumpDrainLimit = 1024u;
// Retail calls the load pump 0x8001231C from eight sites (measured with the decomp pipeline:
// 0x8001039C, 0x800132EC, 0x80013384, 0x80013454, 0x800134C8, 0x80013538, 0x80016CF8, 0x8001E724).
// Seven are load-completion LOOPS that run the pump until FUN_80012ffc reports nothing pending, so
// retail already spends no frames there; 0x8001039C is the only site that runs the pump once per
// FRAME, and that pacing is the whole of the loading-only wait: the pump's own
// `DAT_80050628 = 3` inter-read cooldown costs three frames per queued read.
constexpr std::uint32_t kPerFramePumpReturn = 0x800103A4u;

// The pump's own queue state, the three words FUN_8001231C reads and writes. Retail's pending test
// FUN_80012ffc is their OR, so all three zero is exactly "nothing pending".
struct LoadQueue {
  std::uint32_t cooldown;
  std::uint32_t readActive;
  std::uint32_t queueHead;
};

LoadQueue readLoadQueue(Core &core) {
  return {core.mem_r32(guest::kLoadCooldownWord),
          core.mem_r32(guest::kLoadReadActiveWord),
          core.mem_r32(guest::kLoadQueueHeadWord)};
}

bool loadQueueIdle(const LoadQueue &queue) {
  return queue.cooldown == 0u && queue.readActive == 0u && queue.queueHead == 0u;
}

void loadPumpOwned(Core *core) {
  // The pump advances the load queue exactly ONE step per call — tick the hard-coded inter-read
  // cooldown, poll-complete the active read, or start the next queued read — and returns 0. Retail
  // runs it once per frame at 0x8001039C and in a busy `while (FUN_80012ffc())` loop at its seven
  // other call sites, so only the per-frame pacing is a wait, and only there is more than one step
  // run. Every caller gets its one retail step; only the per-frame site repeats.
  //
  // Only the pump runs, never the two helpers retail pairs with a pump step inside its own loops
  // (0x80010AE8(&DAT_8004e0f0), the frame-heap work drain, and 0x8002BAE8, the pending-completion
  // publish at 0x800654C0/0x800654C4). At the per-frame site retail calls nothing else, so calling
  // them here would publish a completion the guest's loop owner has not asked for; they stay with
  // the loop callers that own them, and the loop keeps driving the queue to quiescence itself.
  //
  // Measured 2026-10-02: a drain that also emptied the queue from the loop callers left the Polar
  // Push briefing refusing Cross — no read was ever requested — because the loop body, and with it
  // the helpers, never ran. Draining the per-frame site leaves every loop caller on retail's own
  // code path, and the keyed Polar Push replay reaches the live match on both the pre- and
  // post-S020 builds.
  //
  // Retail's while condition is read from the three pump-owned words instead of by calling
  // 0x80012FFC: the original-call seam only exists at installed owners, and these words are this
  // owner's own state. A step that leaves all three unchanged means retail's start gate (0x8002BB80)
  // refused to begin a read — a real wait retail's pacing would defer to the next frame, so stop and
  // keep that pacing instead of spinning; the iteration cap bounds the drain outright.
  //
  // `continuation` is where the retail pump returns to. Both seams read r[31]: the original-call
  // seam stops there, and the dispatcher resumes the guest at r[31] when this owner returns.
  const std::uint32_t continuation = core->r[31];
  const bool perFrame = continuation == kPerFramePumpReturn;
  LoadQueue before = readLoadQueue(*core);
  for (std::uint32_t step = 0; step < kLoadPumpDrainLimit; ++step) {
    // Retail calls the pump once per call whatever the queue holds; this owner's extra steps are the
    // removal, and only at the site whose one step per call is the wait.
    const bool moreSteps = perFrame && !loadQueueIdle(before);
    core->r[31] = continuation;
    runtime::callOriginal(*core, runtime::GuestImage::Resident, guest::kLoadPump);
    const LoadQueue after = readLoadQueue(*core);
    if (!moreSteps) {
      break;
    }
    if (loadQueueIdle(after)) {
      break;
    }
    if (before.cooldown == after.cooldown && before.readActive == after.readActive &&
        before.queueHead == after.queueHead) {
      lucent::debug("crashbash-cd",
                    "load pump stalled with queue head 0x{:08X} cooldown {}; deferring to retail pacing",
                    after.queueHead,
                    after.cooldown);
      break;
    }
    before = after;
  }
  core->r[31] = continuation;
  // The retail pump returns 0.
  core->r[2] = 0u;
}

} // namespace

void registerCdFileReadOverride(Core &core) {
  runtime::registerNativeOverride(
      core, runtime::GuestImage::Resident, guest::kCdFileRead, "CrashBash::CdFileRead", cdFileReadOwned);
}

void registerLoadPumpDrainOverride(Core &core) {
  runtime::registerNativeOverride(
      core, runtime::GuestImage::Resident, guest::kLoadPump, "CrashBash::LoadPumpDrain", loadPumpOwned);
}

} // namespace crashbash
