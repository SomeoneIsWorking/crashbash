#pragma once

#include "scene_snapshot.h"

#include <cstdint>

struct Core;

namespace crashbash::render {

// Submit this frame's captured 0x8002992C records in the same order the retail OT would walk them:
// higher bins first, with AddPrim's LIFO order inside each bin.
void submitSpriteQuads(Core &core, const SceneSnapshot &snapshot, std::uint32_t renderList);

} // namespace crashbash::render
