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

void writePackedVertex(Core &core, std::uint32_t address, std::int16_t x) {
  core.mem_w16(address, static_cast<std::uint16_t>(x));
  core.mem_w16(address + 2u, static_cast<std::uint16_t>(x + 1));
  core.mem_w16(address + 4u, static_cast<std::uint16_t>(x + 2));
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
    check(draw.faces[0].sourceVertexAddress == vertices && draw.faces[1].sourceVertexAddress == vertices + 24u,
          "each face retains the exact source vertex cursor used by the shipping decoder");
    check(draw.faces[0].sourceGroup == 0u && draw.faces[0].sourceGroupFace == 0u && draw.faces[1].sourceGroup == 1u &&
              draw.faces[1].sourceGroupFace == 0u,
          "source group identity retains the independent-group restart");
  }

  constexpr std::uint32_t indexedModelData = 0x80020000u;
  constexpr std::uint32_t indexedObject = 0x80020100u;
  constexpr std::uint32_t groupTableRelative = 0x200u;
  constexpr std::uint32_t group = indexedModelData + groupTableRelative + 0x44u;
  constexpr std::uint32_t indexedFrame = 0x80021000u;
  constexpr std::uint32_t animationHandle = 0x80022000u;
  constexpr std::uint32_t animation = 0x80023000u;
  constexpr std::uint32_t vertexPool = 0x80024000u;
  constexpr std::uint32_t vertexIndices = 0x80025000u;
  constexpr std::uint32_t interpolationIndices = 0x80025100u;
  constexpr std::uint32_t indexedTopology = 0x80026000u;
  constexpr std::uint32_t indexedTextureIndices = 0x80026100u;
  constexpr std::uint32_t indexedTextureDescriptors = 0x80026200u;
  constexpr std::uint32_t indexedMaterials = 0x80026300u;
  constexpr std::uint32_t indexedColors = 0x80026400u;
  constexpr std::uint32_t indexedUv = 0x80026500u;

  core.mem_w32(indexedModelData + 0x40u, 1u);
  core.mem_w32(indexedModelData + 0x44u, groupTableRelative);
  core.mem_w32(group + 8u, 1u);
  writeRelative(core, group, 0x0Cu, 0x0Cu, indexedFrame);
  core.mem_w32(group + 0x14u, animationHandle);
  core.mem_w32(animationHandle, animation);
  core.mem_w32(animation, vertexPool - animation);
  writeRelative(core, animation + 4u, 0u, 0x14u, vertexIndices);
  writeRelative(core, animation + 4u, 4u, 0x18u, interpolationIndices);
  core.mem_w32(animation + 0x0Cu, 0x800u);
  writeRelative(core, indexedFrame, 0x14u, 0x14u, indexedTopology);
  writeRelative(core, indexedFrame, 0x18u, 0x18u, indexedTextureIndices);
  writeRelative(core, indexedFrame, 0x1Cu, 0x1Cu, indexedTextureDescriptors);
  writeRelative(core, indexedFrame, 0x20u, 0x20u, indexedMaterials);
  writeRelative(core, indexedModelData, 0x20u, 0x20u, indexedColors);
  writeRelative(core, indexedModelData, 0x24u, 0x24u, indexedUv);

  for (std::uint32_t vertex = 0; vertex < 6u; ++vertex) {
    writePackedVertex(core, vertexPool + vertex * 6u, static_cast<std::int16_t>(100 + vertex * 10u));
  }
  for (std::uint32_t vertex = 0; vertex < 3u; ++vertex) {
    core.mem_w16(vertexIndices + vertex * 2u, static_cast<std::uint16_t>((vertex << 2u) | vertex));
    core.mem_w16(interpolationIndices + vertex * 2u, static_cast<std::uint16_t>(((vertex + 3u) << 2u) | (3u - vertex)));
  }
  core.mem_w8(indexedTopology, 1u);
  core.mem_w8(indexedTopology + 1u, 1u);
  core.mem_w8(indexedTopology + 2u, 0u);
  core.mem_w8(indexedTopology + 3u, 0xFFu);
  core.mem_w16(indexedTextureIndices, 0u);
  core.mem_w16(indexedTextureDescriptors, 0u);
  core.mem_w16(indexedMaterials, 0u);
  core.mem_w32(indexedColors, 0x00101010u);
  core.mem_w32(indexedColors + 4u, 0x00202020u);
  core.mem_w32(indexedColors + 8u, 0x00303030u);

  ModelDraw indexedDraw{
      .object = indexedObject,
      .modelData = indexedModelData,
      .frameCode = 0x4000u,
  };
  const auto indexedCensus = captureFixedModelRecipe(core, indexedDraw);
  check(indexedCensus.status == ModelRecipeStatus::Ready && indexedCensus.faces == 1u,
        "indexed animation-family recipe is decoded");
  check(indexedDraw.faces.size() == 1u, "indexed recipe retains its source face");
  if (indexedDraw.faces.size() == 1u) {
    check(indexedDraw.faces[0].vertices[0].x == 115 && indexedDraw.faces[0].vertices[1].x == 125 &&
              indexedDraw.faces[0].vertices[2].x == 135,
          "indexed recipe expands and blends the two animation vertex streams at IR0");
    check(indexedDraw.faces[0].vertices[0].flags == 3u && indexedDraw.faces[0].vertices[1].flags == 2u &&
              indexedDraw.faces[0].vertices[2].flags == 1u,
          "indexed interpolation preserves retail's second-stream vertex flags");
    check(indexedDraw.faces[0].sourceVertexAddress == vertexIndices,
          "indexed face identity names the authored index-stream cursor");
  }

  if (failures != 0) {
    return 1;
  }
  std::puts("model recipe capture: all checks passed");
  return 0;
}
