// Empty compatibility table for framework algorithms that have not migrated off Core::hooks.
// CrashBashRuntime owns the title's behavior through virtual overrides.
#include "game_iface.h"
#include "legacy_game_interface.h"

static const GameHooks kCrashBashHooks = {};

const GameHooks &crashbash::legacy::compatibilityHooks = kCrashBashHooks;
