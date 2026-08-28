#include "core.h"
#include "game.h"
#include "model_recipe_capture.h"

#include <cstdint>
#include <cstdio>
#include <memory>

namespace {

void writeRelative(Core &core, std::uint32_t base, std::uint32_t field, std::uint32_t addend, std::uint32_t target) {
  core.mem_w32(base + field, target - base - addend);
}

void writeVertex(Core &core, std::uint32_t address, std::int16_t x) {
  core.mem_w16(address, static_cast<std::uint16_t>(x));
  core.mem_w16(address + 2u, static_cast<std::uint16_t>(x + 1));
  core.mem_w16(address + 4u, static_cast<std::uint16_t>(x + 2));
  core.mem_w16(address + 6u, 0u);
}

} // namespace

int main() {
  using crashbash::render::captureFixedModelRecipe;
  using crashbash::render::ModelDraw;
  using crashbash::render::ModelRecipeStatus;

  int failures = 0;
  const auto check = [&failures](bool condition, const char *message) {
    if (!condition) {
      std::fprintf(stderr, "FAIL: %s\n", message);
      ++failures;
    }
  };

  auto game = std::make_unique<Game>();
  Core &core = game->core;
  constexpr std::uint32_t modelData = 0x80010000u;
  constexpr std::uint32_t frame = modelData + 0x24u;
  constexpr std::uint32_t vertices = 0x80011000u;
  constexpr std::uint32_t topology = 0x80012000u;
  constexpr std::uint32_t textureIndices = 0x80013000u;
  constexpr std::uint32_t textureDescriptors = 0x80014000u;
  constexpr std::uint32_t materials = 0x80015000u;
  constexpr std::uint32_t colors = 0x80016000u;
  constexpr std::uint32_t uv = 0x80017000u;

  core.mem_w32(modelData + 0x54u, 0u);
  writeRelative(core, frame, 0x10u, 0x24u, vertices);
  writeRelative(core, frame, 0x14u, 0x14u, topology);
  writeRelative(core, frame, 0x18u, 0x18u, textureIndices);
  writeRelative(core, frame, 0x1Cu, 0x1Cu, textureDescriptors);
  writeRelative(core, frame, 0x20u, 0x20u, materials);
  writeRelative(core, modelData, 0x20u, 0x20u, colors);
  writeRelative(core, modelData, 0x24u, 0x24u, uv);

  for (std::uint32_t vertex = 0; vertex < 6u; ++vertex) {
    writeVertex(core, vertices + vertex * 8u, static_cast<std::int16_t>(10 + vertex * 10u));
  }
  core.mem_w8(topology, 1u);
  core.mem_w8(topology + 1u, 1u);
  core.mem_w8(topology + 2u, 1u);
  core.mem_w8(topology + 3u, 1u);
  core.mem_w8(topology + 4u, 0u);
  core.mem_w8(topology + 5u, 0xFFu);
  core.mem_w16(textureIndices, 0u);
  core.mem_w16(textureIndices + 2u, 0u);
  core.mem_w16(textureDescriptors, 1u << 9u);
  core.mem_w16(materials, 0u);
  core.mem_w16(materials + 2u, 3u);
  for (std::uint32_t color = 0; color < 6u; ++color) {
    core.mem_w32(colors + color * 4u, 0x00101010u + color);
  }

  ModelDraw draw{
      .modelData = modelData,
      .frameCode = 0x2000u,
  };
  const auto census = captureFixedModelRecipe(core, draw);
  check(census.status == ModelRecipeStatus::Ready, "two-group recipe is decoded");
  check(census.groups == 2u && census.faces == 2u, "both topology groups contribute one face");
  check(draw.faces.size() == 2u, "two decoded source faces are retained");
  if (draw.faces.size() == 2u) {
    check(draw.faces[0].vertices[0].x == 10 && draw.faces[0].vertices[1].x == 20 && draw.faces[0].vertices[2].x == 30,
          "first group consumes vertices 0,1,2");
    check(draw.faces[1].vertices[0].x == 40 && draw.faces[1].vertices[1].x == 50 && draw.faces[1].vertices[2].x == 60,
          "second group restarts from its own two priming vertices");
    check(draw.faces[0].sourceFace == 0u && draw.faces[1].sourceFace == 1u,
          "source-face identity remains continuous across groups");
  }

  if (failures != 0) {
    return 1;
  }
  std::puts("model recipe capture: all checks passed");
  return 0;
}
