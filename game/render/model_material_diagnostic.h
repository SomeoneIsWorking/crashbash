#pragma once

#include <array>
#include <cstdint>

namespace crashbash::render {

struct ModelDraw;
struct ModelFace;

struct ModelColorCueInputs {
  std::int16_t factor = 0;
  std::array<std::int32_t, 3> farColor{};
};

struct ModelMaterialSemantics {
  bool semiTransparent = false;
  std::uint8_t blendMode = 0;
};

struct ModelMaterialWitness {
  bool valid = false;
  std::uint32_t object = 0;
  std::uint16_t frameCode = 0;
  std::uint32_t sourceFace = 0;
  std::uint16_t material = 0;
  std::uint8_t topologyFlags = 0;
  std::uint32_t effectiveSubmitFlags = 0;
  std::uint64_t screenAreaTwice = 0;
  std::array<std::array<std::int32_t, 2>, 3> projectedVertices{};
  std::uint32_t sortKey = 0;
  std::array<std::uint32_t, 3> rawColors{};
  std::array<std::uint32_t, 3> retailColors{};
  std::array<std::uint16_t, 3> textureCoordinates{};
  std::uint16_t texturePage = 0;
  std::uint16_t clut = 0;
  bool textured = false;
  bool capturedSemiTransparent = false;
  bool retailSemiTransparent = false;
  std::uint8_t capturedBlendMode = 0;
  std::uint8_t retailBlendMode = 0;
};

struct ModelMaterialFrameCensus {
  std::uint32_t acceptedFaces = 0;
  std::uint32_t rawBlackFaces = 0;
  std::uint32_t dpcsChangedFaces = 0;
  std::uint32_t rawBlackDpcsColoredFaces = 0;
  std::uint32_t forcedSemiTransparentFaces = 0;
  std::uint32_t blendOverrideFaces = 0;
  ModelMaterialWitness largestDpcsChange;
  std::array<ModelMaterialWitness, 4> largestAcceptedFaces{};
  ModelMaterialWitness largestRawBlack;
  ModelMaterialWitness largestRawBlackDpcsColor;
  ModelMaterialWitness largestForcedSemiTransparent;
  ModelMaterialWitness largestBlendOverride;
};

ModelColorCueInputs resolveModelColorCueInputs(bool objectCueEnabled,
                                               std::int16_t objectFactor,
                                               const std::array<std::int32_t, 3> &objectFarColor,
                                               std::int32_t globalFactor,
                                               const std::array<std::int32_t, 3> &globalFarColor);
std::uint32_t applyModelDpcs(std::uint32_t sourceColor, const ModelColorCueInputs &inputs);
ModelMaterialSemantics decodeModelMaterialSemantics(std::uint16_t material, std::uint32_t effectiveSubmitFlags);

// The per-frame material evidence. Diagnostic only: it observes what the native producer decided and
// never supplies it rendering state. One instance per frame-driver, so the census belongs to the run
// that measured it rather than to the process.
class MaterialDiagnostic {
public:
  void beginFrame();
  void observe(const ModelDraw &draw,
               const ModelFace &face,
               const std::array<std::array<std::int32_t, 2>, 3> &projectedVertices,
               std::uint32_t sortKey);
  void reportFrame(std::uint32_t frame);

  const ModelMaterialFrameCensus &frameCensus() const {
    return census_;
  }

private:
  ModelMaterialFrameCensus census_{};
};

} // namespace crashbash::render
