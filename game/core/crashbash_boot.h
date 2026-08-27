#pragma once

class Core;

namespace crashbash {

// Execute the finite startup prefix of retail game main 0x8002718C and application main
// 0x80010158. Their lifetime process-loop frames remain on the guest stack; repetition is owned by
// CrashBashFrameDriver instead of dispatching the non-returning guest process runner 0x800270F0.
void runBootPrefix(Core &core);

} // namespace crashbash
