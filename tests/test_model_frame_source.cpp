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
  constexpr std::uint32_t model = 0x80010000u;

  Memory direct{{model + 0x54u, 3u}};
  assert(crashbash::render::resolveModelFrameSource(0x2002u, model, reader(direct)) == model + 0x8Cu);
  assert(!crashbash::render::resolveModelFrameSource(0x2004u, model, reader(direct)));

  constexpr std::uint32_t tableRelative = 0x100u;
  constexpr std::uint32_t descriptor = model + tableRelative + 0x20u + 2u * 0x0Cu;
  constexpr std::uint32_t lookupRelative = 0x40u;
  Memory indirect{{model + 0x1Cu, tableRelative},
                  {descriptor, 0x80030000u},
                  {descriptor + 4u, lookupRelative},
                  {descriptor + lookupRelative + 4u, 0x80u}};
  assert(crashbash::render::resolveModelFrameSource(0x5003u, model, reader(indirect)) == 0x80030080u);
  indirect[descriptor + lookupRelative + 4u] = 0u;
  assert(!crashbash::render::resolveModelFrameSource(0x5003u, model, reader(indirect)));

  assert(!crashbash::render::resolveModelFrameSource(0x1000u, model, reader(direct)));
  return 0;
}
