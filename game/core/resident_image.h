#pragma once

#include "psx_exe_image.h"
#include "run_ledger.h"

#include <span>

namespace crashbash {

// Requires the exact title manifest fingerprint before publishing the same bytes through psxport.
// The ledger is this load's own report of what it refused and published; it is passed in rather than
// found, because this loader is called before the framework's run-end report and must not depend on
// any ambient state to say what it did.
psx::cpu::PsxExeLoadResult
loadResidentImage(Core &core, std::span<const std::uint8_t> bytes, diagnostics::RunLedger &ledger);

} // namespace crashbash
