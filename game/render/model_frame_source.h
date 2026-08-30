#pragma once

#include <cstdint>
#include <functional>
#include <optional>

namespace crashbash::render {

using ModelFrameWordReader = std::function<std::optional<std::uint32_t>(std::uint32_t)>;

struct ModelFrameResolveInputs {
  std::uint16_t frameCode = 0;
  std::uint32_t modelData = 0;
  std::uint32_t effectiveFlags = 0;
  std::int16_t objectAnimationFrame = 0;
  std::uint16_t objectInterpolationWeight = 0;
  std::uint16_t objectFrameIndex = 0;
};

struct ModelFrameSource {
  std::uint32_t frameRecord = 0;
  std::uint32_t vertexIndexStream = 0;
  std::uint32_t vertexPool = 0;

  bool indexedVertices() const {
    return vertexIndexStream != 0;
  }

  bool operator==(const ModelFrameSource &) const = default;
};

// Resolves the source recipe selected by 0x80019A60. Families 0x1000/0x4000 select an animation
// descriptor, redirect through its frame-record relative pointer, and expand indexed six-byte XYZ
// vertices. Family 0x2000 addresses the inline frame table directly; family 0x5000 follows the
// title's two-level 0x800159C4/0x8001DD20 table. The reader owns address validation so production and
// tests exercise this one implementation.
bool modelFrameFamilySupported(std::uint16_t frameCode);
std::optional<ModelFrameSource> resolveModelFrameSource(const ModelFrameResolveInputs &inputs,
                                                        const ModelFrameWordReader &readWord);

} // namespace crashbash::render
