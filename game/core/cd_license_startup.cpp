#include "cd_license_startup.h"

#include "core.h"
#include "crashbash_guest.h"
#include "disc.h"
#include "game.h"
#include "override_registry.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <lucent/log.h>
#include <string_view>

#ifdef CRASHBASH_HAVE_SUBSTRATE
#include "rec_decls.h"
#endif

namespace crashbash {
namespace {

constexpr std::uint32_t kPassedState = 0u;

struct DiscFileFact {
  std::string_view path;
  std::uint32_t lba;
  std::uint32_t size;
};

constexpr DiscFileFact kDiscFiles[] = {
    {"SCUS_945.70", guest::kDiscExecutableLba, guest::kDiscExecutableSize},
    {"SYSTEM.CNF", guest::kDiscSystemCnfLba, guest::kDiscSystemCnfSize},
    {"CRASHBSH/CRASHBSH.DAT", guest::kDiscDataLba, guest::kDiscDataSize},
};

constexpr std::array<std::uint8_t, 69> kSystemCnf = {
    'B', 'O',  'O',  'T', ' ', '=', ' ', 'c', 'd', 'r',  'o',  'm',  ':', '\\', 'S',  'C', 'U', 'S',
    '_', '9',  '4',  '5', '.', '7', '0', ';', '1', '\t', '\r', '\n', 'T', 'C',  'B',  ' ', '=', ' ',
    '4', '\r', '\n', 'E', 'V', 'E', 'N', 'T', ' ', '=',  ' ',  '1',  '6', '\r', '\n', 'S', 'T', 'A',
    'C', 'K',  ' ',  '=', ' ', '8', '0', '1', 'F', 'F',  'F',  '0',  '0', '\r', '\n',
};

bool hasMeasuredDiscLayout(DiscState &disc) {
  if (!disc_open(&disc) || disc.track_count != 1u) {
    return false;
  }
  const DiscTrackInfo &track = disc.tracks[0];
  if (track.number != 1u || track.lba != 0 || track.sectors != guest::kDiscTrackSectors || track.pregap != 150 ||
      track.pregap_dv != 0 || track.postgap != 0) {
    return false;
  }
  for (const DiscFileFact &fact : kDiscFiles) {
    std::uint32_t lba = 0;
    std::uint32_t size = 0;
    if (!disc_find_file(&disc, fact.path.data(), &lba, &size) || lba != fact.lba || size != fact.size) {
      return false;
    }
  }
  std::array<std::uint8_t, 2048> sector{};
  if (!disc_read_sector(&disc, kDiscFiles[1].lba, sector.data())) {
    return false;
  }
  return std::equal(kSystemCnf.begin(), kSystemCnf.end(), sector.begin());
}

void cdLicenseStartupOwned(Core *core) {
  // Retail 0x8002D4F4 is a 20-state controller sequence which validates the disc through GetTN,
  // GetTD, ReadTOC, GetID and Test before allowing the front end to continue. Its states 8, 9, 13,
  // 14, 15 and 19 use VSync(-1) as a delay clock. State 16 calls 0x8002E0F0, which draws the red
  // copy-protection failure screen and exits through BIOS B0:38; the authentic-disc path instead
  // completes Pause in state 18 and returns to idle state zero.
  if (core->mem_r32(guest::kCdLicenseState) == kPassedState) {
    core->r[2] = kPassedState;
    return;
  }

  DiscState &disc = core->game->disc;
  // The player launcher has already hash-verified the executable and every registered module from one
  // disc. Bind that provisioned identity to the runtime-opened medium using the measured one-track
  // geometry, all three authoritative file extents, and exact SYSTEM.CNF contents. A mismatched
  // medium is a host refusal, never permission to enter the game's failure renderer.
  if (!hasMeasuredDiscLayout(disc)) {
    lucent::error("crashbash-cd", "license startup rejected a disc that does not match SCUS-94570 layout");
    std::abort();
  }

  core->mem_w32(guest::kCdLicenseState, kPassedState);
  core->r[2] = kPassedState;
  lucent::info("crashbash-cd", "license startup accepted measured SCUS-94570 disc identity");
}

} // namespace

void registerCdLicenseStartupOverride() {
#ifdef CRASHBASH_HAVE_SUBSTRATE
  overrides::install(guest::kCdLicenseStartup,
                     "CrashBash::CdLicenseStartup",
                     cdLicenseStartupOwned,
                     gen_func_8002D4F4,
                     shard_set_override);
#else
  lucent::debug("crashbash-cd", "CD license-startup override registration deferred: no generated substrate");
#endif
}

} // namespace crashbash
