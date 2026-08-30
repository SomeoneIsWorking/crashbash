#pragma once

#include "scene_snapshot.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace crashbash::render {

struct ModelPacketPayload {
  std::uint32_t packetNode = 0;
  std::array<std::uint32_t, 10> words{};
};

struct ModelPacketFillObservation {
  std::uint32_t packetBlock = 0;
  std::uint32_t vertexBase = 0;
  std::uint32_t topologyBase = 0;
  std::vector<ModelPacketPayload> payloads;
};

struct ModelPacketGeometryComparison {
  bool valid = false;
  bool projectedCoordinatesMatch = false;
  std::array<std::array<std::int16_t, 2>, 3> packetVertices{};
  std::array<std::array<std::int16_t, 2>, 3> nativeVertices{};
  std::array<std::array<float, 2>, 3> nativeFloatVertices{};
  std::array<std::uint16_t, 3> nativeDepths{};
  std::uint16_t nativeOtz = 0;
  std::int16_t depthScale = 0;
  std::uint32_t nativeSortKey = 0;
  std::uint8_t nativeRejection = 0;
};

struct ModelPacketIdentity {
  std::uint32_t packetNode = 0;
  std::uint32_t packetBlock = 0;
  std::uint32_t object = 0;
  std::uint32_t objectFlags = 0;
  std::uint32_t callFlags = 0;
  std::uint32_t modelAsset = 0;
  std::uint32_t modelData = 0;
  std::uint16_t frameCode = 0;
  std::array<std::int32_t, 3> depthCueFarColor{};
  std::int16_t depthCueFactor = 0;
  ModelSubmitter submitter = ModelSubmitter::Standard;
  std::uint32_t fillVertexBase = 0;
  std::uint32_t fillTopologyBase = 0;
  std::optional<ModelPacketPayload> payload;
  ModelPacketGeometryComparison geometry;
  ModelFace face;
};

struct ModelPacketIdentityScan {
  std::uint32_t packetBlocks = 0;
  std::uint32_t targetComparisons = 0;
  std::vector<ModelPacketIdentity> matches;
};

// Retail 0x80019094 allocates one fixed 0x28-byte packet record per source face and passes the
// block base to 0x800193A8. This pure mapping binds an OT packet tag back to that source record; it
// never reads or changes the native render path.
std::optional<ModelPacketIdentity>
identifyModelPacketNode(const ModelDraw &draw, std::uint32_t packetBlock, std::uint32_t packetNode);
ModelPacketIdentityScan scanModelPacketIdentity(const ModelDraw &draw,
                                                const std::vector<ModelPacketFillObservation> &packetBlocks,
                                                const std::vector<std::uint32_t> &packetNodes);
ModelPacketGeometryComparison compareModelPacketGeometry(const ModelDraw &draw, const ModelPacketIdentity &identity);

std::optional<std::vector<std::uint32_t>> parseModelPacketIdentityTargets(std::string_view text);

void beginModelPacketIdentityDiagnosticFrame();
void beginModelPacketIdentityDraw();
void observeModelPacketBlock(ModelPacketFillObservation observation);
void finishModelPacketIdentityDraw(const ModelDraw &draw);
void reportModelPacketIdentityDiagnosticFrame(std::uint32_t frame);
void registerModelPacketIdentityDiagnosticOverride();

} // namespace crashbash::render
