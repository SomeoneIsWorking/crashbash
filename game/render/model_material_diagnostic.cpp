#include "model_material_diagnostic.h"

#include "scene_snapshot.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <lucent/log.h>

namespace crashbash::render {
namespace {

thread_local ModelMaterialFrameCensus census;

std::int64_t arithmeticShiftRight(std::int64_t value, unsigned shift) {
  const std::int64_t divisor = std::int64_t{1} << shift;
  if (value >= 0) {
    return value / divisor;
  }
  return -((-value + divisor - 1) / divisor);
}

std::uint8_t dpcsChannel(std::uint8_t source, std::int32_t farColor, std::int16_t factor) {
  const std::int32_t sourceFixed = static_cast<std::int32_t>(source) << 4;
  const std::int32_t difference = std::clamp(farColor - sourceFixed, -32768, 32767);
  const std::int64_t interpolated =
      sourceFixed + arithmeticShiftRight(static_cast<std::int64_t>(factor) * difference, 12u);
  return static_cast<std::uint8_t>(std::clamp<std::int64_t>(arithmeticShiftRight(interpolated, 4u), 0, 255));
}

bool blackRgb(const std::array<std::uint32_t, 3> &colors) {
  return std::all_of(colors.begin(), colors.end(), [](std::uint32_t color) {
    return (color & 0x00FFFFFFu) == 0;
  });
}

void recordLargest(ModelMaterialWitness &witness,
                   const ModelDraw &draw,
                   const ModelFace &face,
                   const std::array<std::array<std::int32_t, 2>, 3> &projectedVertices,
                   std::uint32_t sortKey) {
  const std::int64_t edgeX1 = static_cast<std::int64_t>(projectedVertices[1][0]) - projectedVertices[0][0];
  const std::int64_t edgeY1 = static_cast<std::int64_t>(projectedVertices[1][1]) - projectedVertices[0][1];
  const std::int64_t edgeX2 = static_cast<std::int64_t>(projectedVertices[2][0]) - projectedVertices[0][0];
  const std::int64_t edgeY2 = static_cast<std::int64_t>(projectedVertices[2][1]) - projectedVertices[0][1];
  const std::int64_t signedAreaTwice = edgeX1 * edgeY2 - edgeY1 * edgeX2;
  const std::uint64_t screenAreaTwice =
      static_cast<std::uint64_t>(signedAreaTwice < 0 ? -signedAreaTwice : signedAreaTwice);
  if (witness.valid && witness.screenAreaTwice >= screenAreaTwice) {
    return;
  }
  witness = {
      .valid = true,
      .object = draw.object,
      .frameCode = draw.frameCode,
      .sourceFace = face.sourceFace,
      .material = face.sourceMaterial,
      .topologyFlags = face.topologyFlags,
      .effectiveSubmitFlags = draw.objectFlags | draw.callFlags,
      .screenAreaTwice = screenAreaTwice,
      .projectedVertices = projectedVertices,
      .sortKey = sortKey,
      .rawColors = face.colors,
      .retailColors = face.retailColors,
      .textureCoordinates = face.textureCoordinates,
      .texturePage = face.texturePage,
      .clut = face.clut,
      .textured = face.textured,
      .capturedSemiTransparent = face.semiTransparent,
      .retailSemiTransparent = face.retailSemiTransparent,
      .capturedBlendMode = face.blendMode,
      .retailBlendMode = face.retailBlendMode,
  };
}

ModelMaterialWitness makeWitness(const ModelDraw &draw,
                                 const ModelFace &face,
                                 const std::array<std::array<std::int32_t, 2>, 3> &projectedVertices,
                                 std::uint32_t sortKey) {
  ModelMaterialWitness witness;
  recordLargest(witness, draw, face, projectedVertices, sortKey);
  return witness;
}

void recordLargestAccepted(const ModelMaterialWitness &candidate) {
  auto position = std::find_if(census.largestAcceptedFaces.begin(),
                               census.largestAcceptedFaces.end(),
                               [&candidate](const ModelMaterialWitness &existing) {
                                 return !existing.valid || candidate.screenAreaTwice > existing.screenAreaTwice;
                               });
  if (position == census.largestAcceptedFaces.end()) {
    return;
  }
  std::move_backward(position, census.largestAcceptedFaces.end() - 1, census.largestAcceptedFaces.end());
  *position = candidate;
}

} // namespace

ModelColorCueInputs resolveModelColorCueInputs(bool objectCueEnabled,
                                               std::int16_t objectFactor,
                                               const std::array<std::int32_t, 3> &objectFarColor,
                                               std::int32_t globalFactor,
                                               const std::array<std::int32_t, 3> &globalFarColor) {
  ModelColorCueInputs inputs;
  if (globalFactor == 0) {
    if (objectCueEnabled) {
      inputs.factor = objectFactor;
      inputs.farColor = objectFarColor;
    }
    return inputs;
  }

  inputs.factor = static_cast<std::int16_t>(globalFactor);
  if (!objectCueEnabled) {
    inputs.farColor = globalFarColor;
    return inputs;
  }

  const std::int64_t scale = 0x1000 - static_cast<std::int64_t>(globalFactor);
  for (std::uint32_t component = 0; component < inputs.farColor.size(); ++component) {
    inputs.farColor[component] =
        static_cast<std::int32_t>(arithmeticShiftRight(scale * objectFarColor[component], 12u));
  }
  return inputs;
}

std::uint32_t applyModelDpcs(std::uint32_t sourceColor, const ModelColorCueInputs &inputs) {
  const std::uint32_t red = dpcsChannel(static_cast<std::uint8_t>(sourceColor), inputs.farColor[0], inputs.factor);
  const std::uint32_t green =
      dpcsChannel(static_cast<std::uint8_t>(sourceColor >> 8u), inputs.farColor[1], inputs.factor);
  const std::uint32_t blue =
      dpcsChannel(static_cast<std::uint8_t>(sourceColor >> 16u), inputs.farColor[2], inputs.factor);
  return (sourceColor & 0xFF000000u) | red | (green << 8u) | (blue << 16u);
}

ModelMaterialSemantics decodeModelMaterialSemantics(std::uint16_t material, std::uint32_t effectiveSubmitFlags) {
  const bool forced = (effectiveSubmitFlags & 1u) != 0;
  return {
      .semiTransparent = forced || (material & 0x8000u) != 0,
      .blendMode = static_cast<std::uint8_t>((forced ? effectiveSubmitFlags >> 5u : material >> 13u) & 3u),
  };
}

void beginModelMaterialDiagnosticFrame() {
  census = {};
}

void observeModelMaterialFace(const ModelDraw &draw,
                              const ModelFace &face,
                              const std::array<std::array<std::int32_t, 2>, 3> &projectedVertices,
                              std::uint32_t sortKey) {
  ++census.acceptedFaces;
  const ModelMaterialWitness candidate = makeWitness(draw, face, projectedVertices, sortKey);
  recordLargestAccepted(candidate);
  const bool rawBlack = blackRgb(face.colors);
  const bool dpcsChanged = face.colors != face.retailColors;
  const bool rawBlackDpcsColored = rawBlack && !blackRgb(face.retailColors);
  const bool forcedSemiTransparent = !face.semiTransparent && face.retailSemiTransparent;
  const bool blendOverride = face.blendMode != face.retailBlendMode;

  if (rawBlack) {
    ++census.rawBlackFaces;
    recordLargest(census.largestRawBlack, draw, face, projectedVertices, sortKey);
  }
  if (dpcsChanged) {
    ++census.dpcsChangedFaces;
    recordLargest(census.largestDpcsChange, draw, face, projectedVertices, sortKey);
  }
  if (rawBlackDpcsColored) {
    ++census.rawBlackDpcsColoredFaces;
    recordLargest(census.largestRawBlackDpcsColor, draw, face, projectedVertices, sortKey);
  }
  if (forcedSemiTransparent) {
    ++census.forcedSemiTransparentFaces;
    recordLargest(census.largestForcedSemiTransparent, draw, face, projectedVertices, sortKey);
  }
  if (blendOverride) {
    ++census.blendOverrideFaces;
    recordLargest(census.largestBlendOverride, draw, face, projectedVertices, sortKey);
  }
}

const ModelMaterialFrameCensus &modelMaterialFrameCensus() {
  return census;
}

void reportModelMaterialDiagnosticFrame(std::uint32_t frame) {
  const ModelMaterialWitness &dpcs = census.largestDpcsChange;
  const ModelMaterialWitness &black = census.largestRawBlack;
  const ModelMaterialWitness &semi = census.largestForcedSemiTransparent;
  const ModelMaterialWitness &blend = census.largestBlendOverride;
  lucent::debug("crashbash-material",
                "f{} accepted={} raw-black={} dpcs-changed={} raw-black->color={} forced-semi={} blend-override={}; "
                "dpcs=(valid={},obj={:08X},frame={:04X},face={},mat={:04X},topo={:02X},flags={:08X},area={},raw={:08X}/"
                "{:08X}/{:08X},retail={:08X}/{:08X}/{:08X}) black=(valid={},obj={:08X},face={},mat={:04X},flags={:08X},"
                "area={},raw={:08X}/{:08X}/{:08X},retail={:08X}/{:08X}/{:08X}) semi=(valid={},obj={:08X},face={},"
                "mat={:04X},flags={:08X},captured={}/{},retail={}/{},area={}) blend=(valid={},obj={:08X},face={},"
                "mat={:04X},flags={:08X},captured={}/{},retail={}/{},area={})",
                frame,
                census.acceptedFaces,
                census.rawBlackFaces,
                census.dpcsChangedFaces,
                census.rawBlackDpcsColoredFaces,
                census.forcedSemiTransparentFaces,
                census.blendOverrideFaces,
                dpcs.valid,
                dpcs.object,
                dpcs.frameCode,
                dpcs.sourceFace,
                dpcs.material,
                dpcs.topologyFlags,
                dpcs.effectiveSubmitFlags,
                dpcs.screenAreaTwice,
                dpcs.rawColors[0],
                dpcs.rawColors[1],
                dpcs.rawColors[2],
                dpcs.retailColors[0],
                dpcs.retailColors[1],
                dpcs.retailColors[2],
                black.valid,
                black.object,
                black.sourceFace,
                black.material,
                black.effectiveSubmitFlags,
                black.screenAreaTwice,
                black.rawColors[0],
                black.rawColors[1],
                black.rawColors[2],
                black.retailColors[0],
                black.retailColors[1],
                black.retailColors[2],
                semi.valid,
                semi.object,
                semi.sourceFace,
                semi.material,
                semi.effectiveSubmitFlags,
                semi.capturedSemiTransparent,
                semi.capturedBlendMode,
                semi.retailSemiTransparent,
                semi.retailBlendMode,
                semi.screenAreaTwice,
                blend.valid,
                blend.object,
                blend.sourceFace,
                blend.material,
                blend.effectiveSubmitFlags,
                blend.capturedSemiTransparent,
                blend.capturedBlendMode,
                blend.retailSemiTransparent,
                blend.retailBlendMode,
                blend.screenAreaTwice);

  for (std::uint32_t rank = 0; rank < census.largestAcceptedFaces.size(); ++rank) {
    const ModelMaterialWitness &face = census.largestAcceptedFaces[rank];
    lucent::debug("crashbash-material-face",
                  "f{} rank={} valid={} obj={:08X} frame={:04X} face={} mat={:04X} topo={:02X} flags={:08X} "
                  "area={} xy={}/{}:{}/{}:{}/{} sort={} textured={} tpage={:04X} clut={:04X} uv={:04X}/{:04X}/"
                  "{:04X} native={:08X}/{:08X}/{:08X} retail={:08X}/{:08X}/{:08X} semi={}->{} blend={}->{}",
                  frame,
                  rank,
                  face.valid,
                  face.object,
                  face.frameCode,
                  face.sourceFace,
                  face.material,
                  face.topologyFlags,
                  face.effectiveSubmitFlags,
                  face.screenAreaTwice,
                  face.projectedVertices[0][0],
                  face.projectedVertices[0][1],
                  face.projectedVertices[1][0],
                  face.projectedVertices[1][1],
                  face.projectedVertices[2][0],
                  face.projectedVertices[2][1],
                  face.sortKey,
                  face.textured,
                  face.texturePage,
                  face.clut,
                  face.textureCoordinates[0],
                  face.textureCoordinates[1],
                  face.textureCoordinates[2],
                  face.rawColors[0],
                  face.rawColors[1],
                  face.rawColors[2],
                  face.retailColors[0],
                  face.retailColors[1],
                  face.retailColors[2],
                  face.capturedSemiTransparent,
                  face.retailSemiTransparent,
                  face.capturedBlendMode,
                  face.retailBlendMode);
  }
}

} // namespace crashbash::render
