#include "cd_startup.h"

#include "core.h"
#include "crashbash_guest.h"
#include "disc.h"
#include "game.h"
#include "guest_execution.h"

#include <lucent/log.h>

namespace crashbash {
namespace {

void cdDriveReadyOwned(Core *core) {
  // Retail 0x800349AC asks libcd for GetTN status and optionally waits in VSync(30) retry loops until
  // status byte 2 says the drive is ready. The generic no-controller command path intentionally has
  // no fabricated status packet, so that body cannot establish readiness. Query the real native CHD
  // backend instead: a parsed non-empty TOC is the host-owned proof that disc operations can run.
  DiscState &disc = core->game->disc;
  core->r[2] = disc_open(&disc) && disc.track_count != 0u ? 2u : 5u;
}

void cdInitHandshakeOwned(Core *core) {
  // Retail 0x80034B8C enters Sony libcd's controller/IRQ initialization and returns one only after
  // the controller is ready. The port has no emulated CD controller: all configured commands,
  // synchronization and ISO lookup complete synchronously on the host. Report that real native
  // readiness to its guest caller, which still installs Crash Bash's callback pointers.
  core->r[2] = 1u;
}

} // namespace

void registerCdStartupOverride(Core &core) {
  runtime::registerNativeOverride(
      core, runtime::GuestImage::Resident, guest::kCdDriveReady, "CrashBash::CdDriveReady", cdDriveReadyOwned);
  runtime::registerNativeOverride(
      core, runtime::GuestImage::Resident, guest::kCdInitHandshake, "CrashBash::CdInitHandshake", cdInitHandshakeOwned);
}

} // namespace crashbash
