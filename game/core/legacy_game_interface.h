#pragma once

struct GameConfig;
struct GameHooks;

namespace crashbash::legacy {

// Compatibility facts for generic framework algorithms that still read Core::cfg/Core::hooks.
// CrashBashRuntime is the title's behavior owner; no new behavior belongs in these tables.
extern const GameConfig &measuredConfig;
extern const GameHooks &compatibilityHooks;

} // namespace crashbash::legacy
