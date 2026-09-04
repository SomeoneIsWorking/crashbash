#pragma once

class Core;

namespace crashbash::diagnostics {

// Install behavior-preserving observers for the nested-MENU accept path and its measured
// DAT28136 successor module.
void registerMenuBoundary(Core &core);

} // namespace crashbash::diagnostics
