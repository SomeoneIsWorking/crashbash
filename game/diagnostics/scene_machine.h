#pragma once

class Core;

namespace crashbash::diagnostics {

// Read-only observation of the retail scene machine.
//
// WHY THIS EXISTS, and what it is NOT. The product's mode question — "which mode is the game in,
// and what did the guest ask to switch to?" — was being read from `kAppModeVtable`
// (`0x8004E0DC`), whose only writer is `0x800101CC` in the resident application main and which
// therefore holds the shell's own root scene for the whole run. That word cannot change, so it is a
// dead tap: it cannot distinguish "the game reached Crashball" from "the game never left the logo",
// and both read the same. The machine that does change is `0x8001E588`/`0x8001E610` over the scene
// record at `0x8009F658` (BOOT's own update, `0x80092BA0`, is
// `func_0x8001e598(&0x8009F644); func_0x8001e610(&0x8009F658,&0x8009F644)`).
//
// `0x8001E588` is a four-instruction leaf — `sw $a1,4($a0); sw $a2,0xc($a0); jr $ra;
// sw $zero,8($a0)` — so observing it costs one original call and changes nothing. This owner
// NEVER writes the scene record, the clock, or the transition flags: the guest takes its own
// transition, and this only says which one it asked for and whether the machine accepted it.
void registerSceneMachine(Core &core);

} // namespace crashbash::diagnostics
