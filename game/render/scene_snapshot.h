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

// Source-level input to Crash Bash's screen-aligned textured and Gouraud-color quad leaves. The record
// is decoded before the retained retail body allocates or writes a GPU packet; it records the target
// render-list identity but no OT contents, packet address, or GP0 output, and remains valid after the
// guest packet pool is recycled.
struct SpriteQuadDraw {
  std::uint32_t sourceFunction = 0;
  std::uint32_t sourceAddress = 0;
  std::uint32_t renderList = 0;
  std::uint32_t packedPosition = 0;
  std::int32_t orderingBin = 0;
  std::array<std::uint32_t, 4> sourceColors{};
  std::array<std::int32_t, 4> x{};
  std::array<std::int32_t, 4> y{};
  std::array<std::uint8_t, 4> u{};
  std::array<std::uint8_t, 4> v{};
  std::array<std::uint8_t, 4> red{};
  std::array<std::uint8_t, 4> green{};
  std::array<std::uint8_t, 4> blue{};
  std::uint16_t texturePage = 0;
  std::uint16_t clut = 0;
  std::uint8_t blendMode = 0;
  bool textured = true;
  bool gouraud = false;
  bool dither = false;
  bool semiTransparent = false;
  bool authoredWorldOrder = false;
};

// GPU draw state sampled at the real native-model submission boundary. DisplayFrame flips the guest
// framebuffer immediately afterwards, so a present-time midpoint must not read the now-current GPU
// offset/clip state or it renders into the opposite 256-line buffer.
struct ModelRenderEnvironment {
  std::int32_t drawOffsetX = 0;
  std::int32_t drawOffsetY = 0;
  std::int32_t drawAreaX0 = 0;
  std::int32_t drawAreaY0 = 0;
  std::int32_t drawAreaX1 = -1;
  std::int32_t drawAreaY1 = -1;
  std::int32_t textureWindowMaskX = 0;
  std::int32_t textureWindowMaskY = 0;
  std::int32_t textureWindowOffsetX = 0;
  std::int32_t textureWindowOffsetY = 0;
  std::int32_t textureDither = 0;
  bool authoredScreenPresentation = false;
  bool valid = false;
};

struct SceneSnapshot {
  std::uint32_t logicFrame = 0;
  bool valid = false;
  bool authoredScreenPresentation = false;
  ModelRenderEnvironment modelEnvironment;
  std::vector<ModelDraw> models;
  std::vector<SpriteQuadDraw> spriteQuads;
};

// Three slots preserve the two completed snapshots needed for temporal presentation while a new
// snapshot is being captured. The guest displays the ordering table built from presentable while it
// builds current; previous and presentable are therefore the exact bounding simulation states for an
// inserted frame.
class SceneSnapshotHistory {
public:
  void beginFrame(std::uint32_t logicFrame);
  ModelDraw &record(ModelDraw draw);
  SpriteQuadDraw &record(SpriteQuadDraw draw);

  SceneSnapshot &presentable();
  const SceneSnapshot &presentable() const;
  const SceneSnapshot &previous() const;
  const SceneSnapshot &current() const;
  bool temporalPairValid() const;
  void markPresented();
  void markUnpresented();

private:
  SceneSnapshot previous_;
  SceneSnapshot presentable_;
  SceneSnapshot current_;
  bool temporalContinuous_ = false;
};

} // namespace crashbash::render
