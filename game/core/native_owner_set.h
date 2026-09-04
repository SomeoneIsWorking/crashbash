#pragma once

class Core;

namespace crashbash {

// Installs the complete title-owned behavior set into one runtime instance. Image-qualified
// registration remains behind guest_execution.h until the typed psxport title adapter exists.
void registerNativeOwners(Core &core);

} // namespace crashbash
