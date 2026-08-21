// Sole adapter between psxport and Crash Bash's generated resident substrate.
#include "core.h"
#include "recomp_iface.h"
#include <cstdlib>
#include <lucent/log.h>

#ifdef CRASHBASH_HAVE_SUBSTRATE
#include "overlay_table.h"

extern void shard_set_override(uint32_t, void (*)(Core *));

static const RecompRegistry kCrashBashRecomp = {
    .main_dispatch = main_dispatch,
    .rec_func_index = rec_func_index,
    .overlays = g_rec_overlays,
    .overlay_count = g_rec_overlay_count,
    .shard_set_override = shard_set_override,
};
#endif

void crashbash_install_recomp() {
#ifdef CRASHBASH_HAVE_SUBSTRATE
  psxport_install_recomp(&kCrashBashRecomp);
#else
  lucent::error("recomp", "Crash Bash generated substrate is absent; run tools/recomp_bootstrap.py --ensure");
  std::abort();
#endif
}
