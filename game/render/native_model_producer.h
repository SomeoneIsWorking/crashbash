#pragma once

#include "scene_snapshot.h"

#include <cstdint>

struct Core;

namespace crashbash::render {

std::uint32_t submitFixedModel(Core &core, const ModelDraw &draw);

} // namespace crashbash::render
