#include "model_recipe_capture.h"

#include "core.h"
#include "model_frame_source.h"

#include <array>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace crashbash::render {
namespace {

constexpr std::uint32_t kRamBegin = 0x80000000u;
constexpr std::uint32_t kRamEnd = 0x80200000u;
constexpr std::uint32_t kFrameFamilyMask = 0x7000u;
constexpr std::uint32_t kDirectFrameFamily = 0x2000u;
constexpr std::uint32_t kResolvedFrameFamily = 0x5000u;
constexpr std::uint32_t kFrameRecordStride = 0x34u;
constexpr std::uint32_t kMaxGroups = 4096u;
constexpr std::uint32_t kMaxFaces = 65536u;

bool ramRange(std::uint32_t address, std::uint32_t size) {
  return address >= kRamBegin && address <= kRamEnd && size <= kRamEnd - address;
}

bool relativeTarget(
    Core &core, std::uint32_t base, std::uint32_t relativeField, std::uint32_t addend, std::uint32_t &target) {
  if (!ramRange(base + relativeField, 4u)) {
    return false;
  }
  const auto relative = static_cast<std::int32_t>(core.mem_r32(base + relativeField));
  const std::int64_t result = static_cast<std::int64_t>(base) + relative + addend;
  if (result < kRamBegin || result >= kRamEnd) {
    return false;
  }
  target = static_cast<std::uint32_t>(result);
  return true;
}

ModelVertex readVertex(Core &core, std::uint32_t address) {
  return ModelVertex{
      .x = static_cast<std::int16_t>(core.mem_r16(address)),
      .y = static_cast<std::int16_t>(core.mem_r16(address + 2u)),
      .z = static_cast<std::int16_t>(core.mem_r16(address + 4u)),
      .flags = core.mem_r16(address + 6u),
  };
}

bool decodeTextureMaterial(Core &core,
                           const ModelDraw &draw,
                           std::uint16_t material,
                           std::uint16_t descriptor,
                           std::uint16_t coordinateIndex,
                           std::uint32_t uvTable,
                           ModelFace &face) {
  if (!ramRange(draw.modelAsset, 0x20u)) {
    return false;
  }

  const std::uint32_t textureBase = core.mem_r32(draw.modelAsset + 0x18u);
  std::uint32_t textureState = 0;
  std::uint16_t clut = 0;
  if ((descriptor & 0x8000u) == 0) {
    const std::uint64_t recordAddress =
        static_cast<std::uint64_t>(textureBase) + static_cast<std::uint64_t>(descriptor & 0x01FFu) * 0x38u;
    if (recordAddress > std::numeric_limits<std::uint32_t>::max() ||
        !ramRange(static_cast<std::uint32_t>(recordAddress), 0x2Au)) {
      return false;
    }
    const std::uint32_t record = static_cast<std::uint32_t>(recordAddress);
    textureState = record + 0x18u;
    clut = core.mem_r16(record + 0x26u);
  } else {
    const std::uint32_t clutBase = core.mem_r32(draw.modelAsset + 0x1Cu);
    const std::uint32_t textureSlot = core.mem_r32(draw.modelAsset + 0x10u);
    const std::uint64_t stateAddress =
        static_cast<std::uint64_t>(textureBase) + static_cast<std::uint64_t>(textureSlot) * 0x38u;
    const std::uint64_t clutAddress =
        static_cast<std::uint64_t>(clutBase) + static_cast<std::uint64_t>(descriptor & 0x01FFu) * 0x0Cu + 2u;
    if (stateAddress < 0x20u || stateAddress - 0x20u > std::numeric_limits<std::uint32_t>::max() ||
        clutAddress > std::numeric_limits<std::uint32_t>::max() ||
        !ramRange(static_cast<std::uint32_t>(stateAddress - 0x20u), 0x12u) ||
        !ramRange(static_cast<std::uint32_t>(clutAddress), 2u)) {
      return false;
    }
    textureState = static_cast<std::uint32_t>(stateAddress - 0x20u);
    clut = core.mem_r16(static_cast<std::uint32_t>(clutAddress));
  }

  const std::uint64_t coordinates = static_cast<std::uint64_t>(uvTable) + coordinateIndex * 2u;
  if (coordinates > std::numeric_limits<std::uint32_t>::max() ||
      !ramRange(static_cast<std::uint32_t>(coordinates), 6u)) {
    return false;
  }
  const std::uint16_t uvBase = core.mem_r16(textureState + 0x10u);
  const std::uint32_t coordinateAddress = static_cast<std::uint32_t>(coordinates);
  for (std::uint32_t i = 0; i < face.textureCoordinates.size(); ++i) {
    face.textureCoordinates[i] = uvBase | core.mem_r16(coordinateAddress + i * 2u);
  }
  face.texturePage =
      static_cast<std::uint16_t>((core.mem_r16(textureState + 0x0Cu) & 0xFF9Fu) | ((material >> 8u) & 0x60u));
  face.clut = clut;
  face.textured = true;
  return true;
}

} // namespace

ModelRecipeCensus captureFixedModelRecipe(Core &core, ModelDraw &draw) {
  ModelRecipeCensus census{};
  draw.faces.clear();
  draw.texturedFaces = 0;
  const std::uint32_t frameFamily = draw.frameCode & kFrameFamilyMask;
  if (frameFamily != kDirectFrameFamily && frameFamily != kResolvedFrameFamily) {
    return census;
  }
  census.status = ModelRecipeStatus::InvalidSource;
  const auto readWord = [&core](std::uint32_t address) -> std::optional<std::uint32_t> {
    return ramRange(address, 4u) ? std::optional<std::uint32_t>(core.mem_r32(address)) : std::nullopt;
  };
  const std::optional<std::uint32_t> frame = resolveModelFrameSource(draw.frameCode, draw.modelData, readWord);
  if (!frame || !ramRange(*frame, kFrameRecordStride)) {
    return census;
  }

  std::uint32_t vertices = 0;
  std::uint32_t topology = 0;
  std::uint32_t materials = 0;
  std::uint32_t textureIndices = 0;
  std::uint32_t textureDescriptors = 0;
  std::uint32_t colorTable = 0;
  std::uint32_t uvTable = 0;
  if (!relativeTarget(core, *frame, 0x10u, 0x24u, vertices) || !relativeTarget(core, *frame, 0x14u, 0x14u, topology) ||
      !relativeTarget(core, *frame, 0x18u, 0x18u, textureIndices) ||
      !relativeTarget(core, *frame, 0x1Cu, 0x1Cu, textureDescriptors) ||
      !relativeTarget(core, *frame, 0x20u, 0x20u, materials) ||
      !relativeTarget(core, draw.modelData, 0x20u, 0x20u, colorTable) ||
      !relativeTarget(core, draw.modelData, 0x24u, 0x24u, uvTable)) {
    return census;
  }

  std::vector<ModelFace> faces;
  std::uint16_t textureDescriptor = 0;
  std::uint32_t descriptorFacesRemaining = 0;
  for (std::uint32_t group = 0; group < kMaxGroups; ++group) {
    if (!ramRange(topology, 2u)) {
      return census;
    }
    const std::uint8_t flags = core.mem_r8(topology);
    const std::uint8_t count = core.mem_r8(topology + 1u);
    topology += 2u;
    if (count == 0xFFu) {
      draw.faces = std::move(faces);
      draw.texturedFaces = census.texturedFaces;
      census.status = draw.faces.empty() ? ModelRecipeStatus::ValidEmpty : ModelRecipeStatus::Ready;
      return census;
    }
    ++census.groups;
    if (count > kMaxFaces - census.faces || !ramRange(vertices, (static_cast<std::uint32_t>(count) + 2u) * 8u) ||
        !ramRange(materials, static_cast<std::uint32_t>(count) * 2u)) {
      return census;
    }
    for (std::uint32_t faceIndex = 0; faceIndex < count; ++faceIndex) {
      if (!ramRange(textureIndices, 2u)) {
        return census;
      }
      const std::uint16_t textureIndex = core.mem_r16(textureIndices);
      textureIndices += 2u;
      if (descriptorFacesRemaining == 0) {
        if (!ramRange(textureDescriptors, 2u)) {
          return census;
        }
        textureDescriptor = core.mem_r16(textureDescriptors);
        textureDescriptors += 2u;
        descriptorFacesRemaining = ((textureDescriptor >> 9u) & 0x3Fu) + 1u;
      }
      --descriptorFacesRemaining;

      const std::uint16_t material = core.mem_r16(materials);
      materials += 2u;
      ++census.faces;
      const std::uint32_t colorIndex = material & 0x1FFFu;
      const std::uint64_t colors = static_cast<std::uint64_t>(colorTable) + colorIndex * 4u;
      if (colors > std::numeric_limits<std::uint32_t>::max() || !ramRange(static_cast<std::uint32_t>(colors), 12u)) {
        return census;
      }
      const std::uint32_t faceVertex = vertices + faceIndex * 8u;
      ModelFace face{
          .vertices = {readVertex(core, faceVertex),
                       readVertex(core, faceVertex + 8u),
                       readVertex(core, faceVertex + 16u)},
          .colors = {core.mem_r32(static_cast<std::uint32_t>(colors)),
                     core.mem_r32(static_cast<std::uint32_t>(colors) + 4u),
                     core.mem_r32(static_cast<std::uint32_t>(colors) + 8u)},
          .semiTransparent = (material & 0x8000u) != 0,
          .blendMode = static_cast<std::uint8_t>((material >> 13u) & 3u),
      };
      if ((flags & 1u) == 0) {
        ++census.texturedFaces;
        if (!decodeTextureMaterial(core, draw, material, textureDescriptor, textureIndex, uvTable, face)) {
          return census;
        }
      }
      faces.push_back(face);
    }
    vertices += (static_cast<std::uint32_t>(count) + 2u) * 8u;
  }
  return census;
}

} // namespace crashbash::render
