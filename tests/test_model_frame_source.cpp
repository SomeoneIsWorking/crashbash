#include "model_frame_source.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace {

using Memory = std::unordered_map<std::uint32_t, std::uint32_t>;

crashbash::render::ModelFrameWordReader reader(const Memory &memory) {
  return [&memory](std::uint32_t address) -> std::optional<std::uint32_t> {
    const auto value = memory.find(address);
    return value == memory.end() ? std::nullopt : std::optional<std::uint32_t>(value->second);
  };
}

} // namespace

int main() {
  using crashbash::render::ModelFrameResolveInputs;
  using crashbash::render::ModelFrameSource;
  using crashbash::render::resolveModelFrameSource;

  constexpr std::uint32_t model = 0x80010000u;

  Memory direct{{model + 0x54u, 3u}};
  assert(resolveModelFrameSource({.frameCode = 0x2002u, .modelData = model}, reader(direct)) ==
         ModelFrameSource{.frameRecord = model + 0x8Cu});
  assert(!resolveModelFrameSource({.frameCode = 0x2004u, .modelData = model}, reader(direct)));

  constexpr std::uint32_t tableRelative = 0x100u;
  constexpr std::uint32_t descriptor = model + tableRelative + 0x20u + 2u * 0x0Cu;
  constexpr std::uint32_t lookupRelative = 0x40u;
  Memory indirect{{model + 0x1Cu, tableRelative},
                  {descriptor, 0x80030000u},
                  {descriptor + 4u, lookupRelative},
                  {descriptor + lookupRelative + 4u, 0x80u}};
  assert(resolveModelFrameSource({.frameCode = 0x5003u, .modelData = model}, reader(indirect)) ==
         ModelFrameSource{.frameRecord = 0x80030080u});
  indirect[descriptor + lookupRelative + 4u] = 0u;
  assert(!resolveModelFrameSource({.frameCode = 0x5003u, .modelData = model}, reader(indirect)));

  constexpr std::uint32_t groupTableRelative = 0x200u;
  constexpr std::uint32_t group = model + groupTableRelative + 0x44u;
  constexpr std::uint32_t animationHandle = 0x80040000u;
  constexpr std::uint32_t animation = 0x80050000u;
  constexpr std::uint32_t frameRecordRelative = 0x300u;
  constexpr std::uint32_t frameRecord = group + frameRecordRelative + 0x0Cu;
  constexpr std::uint32_t vertexPoolRelative = 0x800u;
  constexpr std::uint32_t animationEntry = animation + 4u;
  constexpr std::uint32_t vertexIndicesRelative = 0x100u;
  Memory indexed{{model + 0x40u, 1u},
                 {model + 0x44u, groupTableRelative},
                 {group + 8u, 2u},
                 {group + 0x0Cu, frameRecordRelative},
                 {group + 0x14u, animationHandle},
                 {animationHandle, animation},
                 {animation, vertexPoolRelative},
                 {animationEntry, vertexIndicesRelative},
                 {animationEntry + 8u, 0u}};
  assert((resolveModelFrameSource({.frameCode = 0x4000u, .modelData = model}, reader(indexed)) ==
          ModelFrameSource{.frameRecord = frameRecord,
                           .vertexIndexStream = animationEntry + vertexIndicesRelative + 0x14u,
                           .vertexPool = animation + vertexPoolRelative}));

  indexed[animationEntry + 8u] = 0x20u;
  assert(!resolveModelFrameSource({.frameCode = 0x4000u, .modelData = model}, reader(indexed)));
  indexed[animationEntry + 8u] = 0u;
  const ModelFrameResolveInputs overridden{
      .frameCode = 0x4000u,
      .modelData = model,
      .effectiveFlags = 0x00400000u,
      .objectFrameIndex = 2u,
  };
  assert(resolveModelFrameSource(overridden, reader(indexed))->frameRecord == model + 0x24u + 2u * 0x34u);

  indexed[animation] = 0u;
  indexed[model + 0x28u] = 0x900u;
  assert(resolveModelFrameSource({.frameCode = 0x4000u, .modelData = model}, reader(indexed))->vertexPool ==
         model + 0x928u);

  assert(!crashbash::render::modelFrameFamilySupported(0x3000u));
  assert(!resolveModelFrameSource({.frameCode = 0x3000u, .modelData = model}, reader(direct)));
  return 0;
}
