#include "model_material_diagnostic.h"
#include "scene_snapshot.h"

#include "gte_state.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace {

bool check(bool condition) {
  if (!condition) {
    std::cerr << "model material diagnostic check failed\n";
  }
  return condition;
}

std::uint32_t oracleDpcs(std::uint32_t color, const crashbash::render::ModelColorCueInputs &inputs) {
  GteRegs state{};
  state.REG[6] = color;
  state.REG[8] = static_cast<std::uint16_t>(inputs.factor);
  state.REG[53] = static_cast<std::uint32_t>(inputs.farColor[0]);
  state.REG[54] = static_cast<std::uint32_t>(inputs.farColor[1]);
  state.REG[55] = static_cast<std::uint32_t>(inputs.farColor[2]);
  if (GTE_ExecuteIsolated(&state, 0x4A780010u) < 0) {
    return 0;
  }
  return state.REG[22];
}

} // namespace

int main() {
  using crashbash::render::applyModelDpcs;
  using crashbash::render::beginModelMaterialDiagnosticFrame;
  using crashbash::render::decodeModelMaterialSemantics;
  using crashbash::render::ModelColorCueInputs;
  using crashbash::render::ModelDraw;
  using crashbash::render::ModelFace;
  using crashbash::render::modelMaterialFrameCensus;
  using crashbash::render::observeModelMaterialFace;
  using crashbash::render::resolveModelColorCueInputs;

  bool ok = true;
  const std::array<std::int32_t, 3> objectFar{0x120, 0x340, 0x560};
  const std::array<std::int32_t, 3> globalFar{0x780, 0x9A0, 0xBC0};
  const ModelColorCueInputs disabled = resolveModelColorCueInputs(false, 0x400, objectFar, 0, globalFar);
  ok &= check(disabled.factor == 0 && disabled.farColor == std::array<std::int32_t, 3>{});
  const ModelColorCueInputs object = resolveModelColorCueInputs(true, 0x400, objectFar, 0, globalFar);
  ok &= check(object.factor == 0x400 && object.farColor == objectFar);
  const ModelColorCueInputs global = resolveModelColorCueInputs(false, 0, objectFar, 0x200, globalFar);
  ok &= check(global.factor == 0x200 && global.farColor == globalFar);
  const ModelColorCueInputs scaled = resolveModelColorCueInputs(true, 0, objectFar, 0x800, globalFar);
  ok &= check(scaled.factor == 0x800 && scaled.farColor == std::array<std::int32_t, 3>{0x90, 0x1A0, 0x2B0});

  const auto opaque = decodeModelMaterialSemantics(0x4000u, 0u);
  ok &= check(!opaque.semiTransparent && opaque.blendMode == 2u);
  const auto materialSemi = decodeModelMaterialSemantics(0xA000u, 0u);
  ok &= check(materialSemi.semiTransparent && materialSemi.blendMode == 1u);
  const auto forced = decodeModelMaterialSemantics(0x4000u, 0x61u);
  ok &= check(forced.semiTransparent && forced.blendMode == 3u);

  std::uint32_t random = 0xC001D00Du;
  for (std::uint32_t sample = 0; sample < 512u; ++sample) {
    random ^= random << 13u;
    random ^= random >> 17u;
    random ^= random << 5u;
    const std::uint32_t color = random;
    ModelColorCueInputs inputs{
        .factor = static_cast<std::int16_t>(random >> 16u),
        .farColor = {static_cast<std::int16_t>(random),
                     static_cast<std::int16_t>(random >> 3u),
                     static_cast<std::int16_t>(random >> 7u)},
    };
    ok &= check(applyModelDpcs(color, inputs) == oracleDpcs(color, inputs));
  }

  beginModelMaterialDiagnosticFrame();
  ModelDraw draw{.object = 0x80123456u, .objectFlags = 0x40008000u, .frameCode = 0x2008u};
  ModelFace blackFace{
      .colors = {0u, 0u, 0u},
      .retailColors = {0u, 0u, 0u},
      .sourceFace = 17u,
      .sourceMaterial = 0x8123u,
      .topologyFlags = 8u,
  };
  const std::array<std::array<std::int32_t, 2>, 3> large{{{{0, 0}}, {{20, 0}}, {{0, 10}}}};
  observeModelMaterialFace(draw, blackFace, large, 55u);
  ModelFace coloredFace = blackFace;
  coloredFace.colors = {0x00101010u, 0x00202020u, 0x00303030u};
  coloredFace.retailColors = {0x00010203u, 0x00040506u, 0x00070809u};
  coloredFace.sourceFace = 18u;
  const std::array<std::array<std::int32_t, 2>, 3> small{{{{0, 0}}, {{5, 0}}, {{0, 5}}}};
  observeModelMaterialFace(draw, coloredFace, small, 66u);
  const auto &census = modelMaterialFrameCensus();
  ok &= check(census.acceptedFaces == 2u && census.rawBlackFaces == 1u);
  ok &= check(census.largestAcceptedFaces[0].sourceFace == 17u &&
              census.largestAcceptedFaces[0].screenAreaTwice == 200u && census.largestAcceptedFaces[0].sortKey == 55u);
  ok &=
      check(census.largestAcceptedFaces[1].sourceFace == 18u && census.largestAcceptedFaces[1].screenAreaTwice == 25u);
  ok &= check(census.largestRawBlack.valid && census.largestRawBlack.sourceFace == 17u &&
              census.largestRawBlack.projectedVertices == large);
  return ok ? 0 : 1;
}
