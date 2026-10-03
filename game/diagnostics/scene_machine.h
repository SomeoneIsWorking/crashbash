#pragma once

class Core;

namespace crashbash::diagnostics {

// Read-only observation of the retail scene machine: the leaf `0x8001E588` is four instructions, so
// observing it costs one original call and changes nothing.
//
// It observes `0x8009F658` and its clock `0x8009F644`, the machine that actually selects boot, menu,
// attract and gameplay. `kAppModeVtable` (`0x8004E0DC`) is NOT that machine: it is the shell's own
// root scene, written once before any mode exists, so it cannot distinguish "the game reached
// Crashball" from "the game never left the logo".
//
// This owner NEVER writes the scene record, the clock, or the transition flags: the guest takes its
// own transition, and this only says which one it asked for and whether the machine accepted it.
void registerSceneMachine(Core &core);

} // namespace crashbash::diagnostics
