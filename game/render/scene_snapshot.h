#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace crashbash::render {

using ModelRotation = std::array<std::array<std::int16_t, 3>, 3>;

enum class ModelSubmitter : std::uint32_t {
  Standard = 0x80019F1Cu,
  Alternate = 0x8001DD50u,
};

struct ModelTransform {
  ModelRotation rotation{};
  std::array<std::int32_t, 3> translation{};
  std::int32_t projectionX = 0;
  std::int32_t projectionY = 0;
  std::uint16_t projectionDistance = 0;
  bool valid = false;
};

struct ModelVertex {
  std::int16_t x = 0;
  std::int16_t y = 0;
  std::int16_t z = 0;
  std::uint16_t flags = 0;
};

struct ModelFace {
  std::array<ModelVertex, 3> vertices{};
  std::array<std::uint32_t, 3> colors{};
  std::array<std::uint16_t, 3> textureCoordinates{};
  std::uint16_t texturePage = 0;
  std::uint16_t clut = 0;
  bool textured = false;
  bool semiTransparent = false;
  std::uint8_t blendMode = 0;
  std::array<std::uint32_t, 3> retailColors{};
  std::uint32_t sourceFace = 0;
  std::uint32_t sourceVertexAddress = 0;
  std::uint32_t sourceGroup = 0;
  std::uint32_t sourceGroupFace = 0;
  std::uint16_t sourceMaterial = 0;
  std::uint8_t topologyFlags = 0;
  bool retailSemiTransparent = false;
  std::uint8_t retailBlendMode = 0;
};

// Read-only inputs at Crash Bash's two object-level model submitters. These are captured before
// 0x8001965C installs an object matrix in the guest GTE and before 0x800193A8 writes GPU packets, so
// the native renderer can rebuild the picture from game state rather than transcribing GTE/OT/GP0
// output. `matrix` and `translation` are the explicit standard-submitter arguments; the alternate
// submitter derives them internally and therefore records zero until that derivation is owned.
struct ModelDraw {
  ModelSubmitter submitter = ModelSubmitter::Standard;
  std::uint32_t object = 0;
  std::uint32_t objectFlags = 0;
  std::uint32_t matrix = 0;
  std::uint32_t translation = 0;
  std::uint32_t callFlags = 0;
  std::uint32_t modelAsset = 0;
  std::uint32_t modelData = 0;
  std::uint16_t frameCode = 0;
  std::int16_t depthBias = 0;
  std::int16_t depthLimit = 0;
  std::int16_t depthScale = 0;
  std::array<std::int32_t, 3> depthCueFarColor{};
  std::int16_t depthCueFactor = 0;
  ModelTransform transform;
  std::vector<ModelFace> faces;
  std::uint32_t texturedFaces = 0;
  std::uint32_t nativeFacesSubmitted = 0;
  std::uint32_t nativeZeroDepthRejected = 0;
  std::uint32_t nativeFarDepthRejected = 0;
  std::uint32_t nativeWindingRejected = 0;
};

bool isRenderableModelDraw(const ModelDraw &draw);

struct SceneSnapshot {
  std::uint32_t logicFrame = 0;
  bool valid = false;
  std::vector<ModelDraw> models;
};

// Two immutable-at-present snapshots are the title-owned temporal input. beginFrame rotates the
// completed current snapshot to previous exactly once, then opens a new current snapshot. Model
// submitter overrides append only to the current frame and never mutate guest RAM.
class SceneSnapshotHistory {
public:
  void beginFrame(std::uint32_t logicFrame);
  ModelDraw &record(ModelDraw draw);

  const SceneSnapshot &previous() const;
  const SceneSnapshot &current() const;

private:
  SceneSnapshot previous_;
  SceneSnapshot current_;
};

} // namespace crashbash::render
