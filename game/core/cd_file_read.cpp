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

void loadPumpOwned(Core *core) {
  // The pump advances the load queue exactly ONE step per call — tick the hard-coded inter-read
  // cooldown, poll-complete the active read, or start the next queued read — and returns 0. Drain it
  // to quiescence ONLY at retail's per-frame site, and run only the pump there: at that site retail
  // calls nothing else, so the two helpers retail pairs with a pump step inside its loops
  // (0x80010AE8(&frameHeap), the frame-heap work drain, and 0x8002BAE8, the pending-completion
  // publish) stay with the loop callers that own them. Their call-site return addresses and
  // two-instruction ticks are the emitted loop-body sites 0x8001E72C and 0x8001E734.
  //
  // Draining from the loop callers as well is not merely redundant: measured 2026-10-02, it empties
  // the queue inside the FIRST pump call, so the loop body never runs, and the Polar Push briefing
  // stops responding to Cross altogether — no read is ever requested. At the per-frame site the
  // chain is spread over frames by retail's own cooldown, which is exactly the wait being removed.
  //
  // Retail's while condition is (queue head != 0) + cooldown + read-active, i.e. zero exactly when
  // the three pump-owned words are zero, so read them here instead of calling 0x80012FFC: the
  // original-call seam only exists at installed owners, and these words are this owner's own state.
  // A step that leaves the three words unchanged means retail's start gate (0x8002BB80) refused to
  // begin a read — a real wait the retail pacing would defer to the next frame, so stop and keep that
  // pacing instead of spinning; the iteration cap bounds the drain outright.
  //
  // `continuation` is where the retail pump returns to. Both seams read r[31]: the original-call
  // seam stops there, and the dispatcher resumes the guest at r[31] when this owner returns.
  const std::uint32_t continuation = core->r[31];
  if (continuation == kPerFramePumpReturn) {
    for (std::uint32_t step = 0; step < kLoadPumpDrainLimit; ++step) {
      const std::uint32_t cooldown = core->mem_r32(guest::kLoadCooldownWord);
      const std::uint32_t readActive = core->mem_r32(guest::kLoadReadActiveWord);
      const std::uint32_t queueHead = core->mem_r32(guest::kLoadQueueHeadWord);
      if (cooldown == 0u && readActive == 0u && queueHead == 0u) {
        // Retail's while condition is already zero. The pump body on these three words is a
        // provable no-op, so run neither it nor its per-step helpers.
        break;
      }
      core->r[31] = continuation;
      runtime::callOriginal(*core, runtime::GuestImage::Resident, guest::kLoadPump);
      const std::uint32_t cooldownAfter = core->mem_r32(guest::kLoadCooldownWord);
      const std::uint32_t readActiveAfter = core->mem_r32(guest::kLoadReadActiveWord);
      const std::uint32_t queueHeadAfter = core->mem_r32(guest::kLoadQueueHeadWord);
      if (cooldownAfter == 0u && readActiveAfter == 0u && queueHeadAfter == 0u) {
        break;
      }
      if (cooldown == cooldownAfter && readActive == readActiveAfter && queueHead == queueHeadAfter) {
        lucent::debug("crashbash-cd",
                      "load pump stalled with queue head 0x{:08X} cooldown {}; deferring to retail pacing",
                      queueHeadAfter,
                      cooldownAfter);
        break;
      }
    }
  } else {
    // A load-completion loop: retail's loop body drives this pump to quiescence itself.
    runtime::callOriginal(*core, runtime::GuestImage::Resident, guest::kLoadPump);
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
