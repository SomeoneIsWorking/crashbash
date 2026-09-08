#pragma once

class Core;

namespace crashbash {

// Installs the complete title-owned behavior set into one runtime instance. Image-qualified
// registration binds through the per-Core GuestExecution context composed by TitleAdapter.
void registerNativeOwners(Core &core);

} // namespace crashbash
