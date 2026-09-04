#pragma once

#include <cstdint>

struct Core;

namespace crashbash::render {

// Publish the title's actual GTE scale-factor writes into the framework's per-Core projection owner.
// The integer inputs make the post-write ownership seam directly testable without a second formula.
void publishModelDepthScales(Core &core, std::uint32_t zsf3, std::uint32_t zsf4);
void registerModelDepthScaleCaptureOverride(Core &core);

} // namespace crashbash::render
