#include "crashbash_frame_driver.h"

#include "core.h"
#include "crashbash_guest.h"
#include "game.h"
#include "measured_guest_call.h"
#include "snapshot.h"

#include <cstdlib>
#include <lucent/log.h>

namespace crashbash {

CrashBashFrameDriver::CrashBashFrameDriver(Game &game) : game_(game) {}

void CrashBashFrameDriver::enterProcessState(Core &core, std::uint32_t state) {
  activeState_ = state;
  core.r[16] = state;
  core.mem_w32(guest::kCurrentProcessState, state);
  const std::uint32_t enter = core.mem_r32(state);
  if (enter == 0) {
    lucent::error("crashbash-frame", "process state 0x{:08X} has no enter function", state);
    std::abort();
  }
  measuredGuestCall(core, enter, 0x80027120u, 4u);
  rec_guest_instruction_ticks(&core, 4u);
}

void CrashBashFrameDriver::deliverDisplayFields(Core &core, std::uint32_t fields) {
  if (fields == 0 || fields > 4) {
    lucent::error("crashbash-frame", "retail display requested {} fields; expected a bounded positive cadence", fields);
    std::abort();
  }
  if (deliveredFields_ != 0) {
    lucent::error("crashbash-frame", "retail process iteration invoked DisplayFrame more than once");
    std::abort();
  }
  deliveredFields_ = fields;
  for (std::uint32_t field = 0; field < fields; ++field) {
    const std::uint32_t before = core.mem_r32(guest::kVblankCounter);
    const R3000 interrupted = static_cast<const R3000 &>(core);
    rec_dispatch(&core, guest::kVblankRoot);
    static_cast<R3000 &>(core) = interrupted;
    const std::uint32_t after = core.mem_r32(guest::kVblankCounter);
    if (after != before + 1u) {
      lucent::error("crashbash-frame",
                    "VBlank root 0x{:08X} advanced counter 0x{:08X} by {} instead of one field",
                    guest::kVblankRoot,
                    guest::kVblankCounter,
                    after - before);
      std::abort();
    }
    if (game_.diff_mode) {
      game_.spu_audio.frameLogic();
    } else {
      game_.spu_audio.frame();
    }
  }
}

void CrashBashFrameDriver::stepFrame(Core &core, std::uint32_t frame) {
  game_.timing.logicFrame = frame;
  game_.timing.frameTick();
  core.rsub.otAttr.beginLogicFrame(frame);
  game_.pad.serviceFrame();
  deliveredFields_ = 0;

  std::uint32_t state = core.mem_r32(guest::kCurrentProcessState);
  bool enteredState = false;
  while (state != activeState_) {
    if (state == 0) {
      lucent::error("crashbash-frame", "retail process runner has no current state");
      std::abort();
    }
    if (core.pending_work) {
      rec_irq_poll(&core);
    }
    enterProcessState(core, state);
    enteredState = true;
    const std::uint32_t next = core.mem_r32(guest::kCurrentProcessState);
    if (next == activeState_) {
      break;
    }
    rec_guest_instruction_ticks(&core, 4u);
    state = next;
  }

  // Retail 0x800270F0 checks for a state transition after enter, then executes update and present
  // as one indivisible iteration before observing the next transition.
  if (core.mem_r32(guest::kCurrentProcessState) == activeState_) {
    if (enteredState) {
      core.r[17] = 0x80060000u;
      rec_guest_instruction_ticks(&core, 1u);
    }
    if (core.pending_work) {
      rec_irq_poll(&core);
    }
    const std::uint32_t update = core.mem_r32(activeState_ + 4u);
    const std::uint32_t present = core.mem_r32(activeState_ + 8u);
    if (update == 0 || present == 0) {
      lucent::error("crashbash-frame", "process state 0x{:08X} has an incomplete update/present pair", activeState_);
      std::abort();
    }
    measuredGuestCall(core, update, 0x80027144u, 4u);
    measuredGuestCall(core, present, 0x80027154u, 4u);
    rec_guest_instruction_ticks(&core, 4u);
  }

  snapshot_tick(&core);
  if (deliveredFields_ == 0) {
    game_.presentation.commitUnpresented(&core);
  } else {
    game_.presentation.commit(&core, static_cast<int>(deliveredFields_), game_.temporalPresentation.get());
  }
}

CrashBashFrameDriver &frameDriver(Core &core) {
  if (!core.game || !core.game->frameDriver) {
    lucent::error("crashbash-frame", "Crash Bash runtime has no FrameDriver");
    std::abort();
  }
  return static_cast<CrashBashFrameDriver &>(*core.game->frameDriver);
}

} // namespace crashbash
