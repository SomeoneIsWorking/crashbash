#include "model_packet_identity.h"

#include "model_face_coverage.h"
#include "native_projection.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <string_view>
#include <vector>

namespace crashbash::render {
namespace {

constexpr std::uint32_t kPacketRecordStride = 0x28u;

bool separator(char value) {
  return value == ',' || value == ' ' || value == '\t' || value == '\n';
}

std::array<std::int16_t, 2> unpackSxy(std::uint32_t word) {
  return {
      static_cast<std::int16_t>(word),
      static_cast<std::int16_t>(word >> 16u),
  };
}

} // namespace

std::optional<ModelPacketIdentity>
identifyModelPacketNode(const ModelDraw &draw, std::uint32_t packetBlock, std::uint32_t packetNode) {
  if (packetNode < packetBlock) {
    return std::nullopt;
  }
  const std::uint32_t offset = packetNode - packetBlock;
  if (offset % kPacketRecordStride != 0) {
    return std::nullopt;
  }
  const std::uint32_t faceIndex = offset / kPacketRecordStride;
  if (faceIndex >= draw.faces.size()) {
    return std::nullopt;
  }
  return ModelPacketIdentity{
      .packetNode = packetNode,
      .packetBlock = packetBlock,
      .object = draw.object,
      .objectFlags = draw.objectFlags,
      .callFlags = draw.callFlags,
      .modelAsset = draw.modelAsset,
      .modelData = draw.modelData,
      .frameCode = draw.frameCode,
      .depthCueFarColor = draw.depthCueFarColor,
      .depthCueFactor = draw.depthCueFactor,
      .submitter = draw.submitter,
      .face = draw.faces[faceIndex],
  };
}

ModelPacketIdentityScan scanModelPacketIdentity(const ModelDraw &draw,
                                                const std::vector<ModelPacketFillObservation> &packetBlocks,
                                                const std::vector<std::uint32_t> &packetNodes) {
  ModelPacketIdentityScan scan{
      .packetBlocks = static_cast<std::uint32_t>(packetBlocks.size()),
      .targetComparisons = static_cast<std::uint32_t>(packetBlocks.size() * packetNodes.size()),
  };
  for (const ModelPacketFillObservation &observation : packetBlocks) {
    for (const std::uint32_t packetNode : packetNodes) {
      if (auto identity = identifyModelPacketNode(draw, observation.packetBlock, packetNode)) {
        identity->fillVertexBase = observation.vertexBase;
        identity->fillTopologyBase = observation.topologyBase;
        for (const ModelPacketPayload &payload : observation.payloads) {
          if (payload.packetNode == packetNode) {
            identity->payload = payload;
            break;
          }
        }
        identity->geometry = compareModelPacketGeometry(draw, *identity);
        scan.matches.push_back(*identity);
      }
    }
  }
  return scan;
}

ModelPacketGeometryComparison compareModelPacketGeometry(const ModelDraw &draw, const ModelPacketIdentity &identity) {
  ModelPacketGeometryComparison comparison;
  if (!draw.transform.valid || !identity.payload) {
    return comparison;
  }

  const std::array<std::uint32_t, 3> sxyWords =
      identity.face.textured ? std::array<std::uint32_t, 3>{2u, 5u, 8u} : std::array<std::uint32_t, 3>{4u, 6u, 8u};
  psxport::native_projection::FixedAffine affine{
      .m = draw.transform.rotation,
      .t = draw.transform.translation,
  };
  const psxport::native_projection::ProjectionParams projection{
      .ofx = draw.transform.projectionX,
      .ofy = draw.transform.projectionY,
      .h = draw.transform.projectionDistance,
  };
  std::array<ProjectedFaceVertex, 3> coverageVertices{};
  comparison.valid = true;
  comparison.projectedCoordinatesMatch = true;
  for (std::uint32_t vertexIndex = 0; vertexIndex < 3u; ++vertexIndex) {
    comparison.packetVertices[vertexIndex] = unpackSxy(identity.payload->words[sxyWords[vertexIndex]]);
    const ModelVertex &source = identity.face.vertices[vertexIndex];
    const auto native =
        psxport::native_projection::project(affine, projection, {.x = source.x, .y = source.y, .z = source.z});
    comparison.nativeVertices[vertexIndex] = {native.sx, native.sy};
    comparison.nativeFloatVertices[vertexIndex] = {native.px, native.py};
    comparison.nativeDepths[vertexIndex] = native.sz;
    comparison.projectedCoordinatesMatch &=
        comparison.packetVertices[vertexIndex] == comparison.nativeVertices[vertexIndex];
    coverageVertices[vertexIndex] = {.x = native.sx, .y = native.sy, .depth = native.sz};
  }
  const ModelFaceCoverage coverage = classifyFixedModelFace(coverageVertices,
                                                            identity.face.textured,
                                                            identity.face.vertices[2].flags,
                                                            draw.depthBias,
                                                            draw.depthLimit,
                                                            draw.depthScale);
  comparison.nativeOtz = fixedModelAvsz3Otz(coverageVertices, draw.depthScale);
  comparison.depthScale = draw.depthScale;
  comparison.nativeSortKey = coverage.sortKey;
  comparison.nativeRejection = static_cast<std::uint8_t>(coverage.rejection);
  return comparison;
}

std::array<std::uint32_t, 3> modelPacketColors(const ModelPacketPayload &payload, bool textured) {
  const std::array<std::uint32_t, 3> colorWords =
      textured ? std::array<std::uint32_t, 3>{1u, 4u, 7u} : std::array<std::uint32_t, 3>{3u, 5u, 7u};
  return {payload.words[colorWords[0]], payload.words[colorWords[1]], payload.words[colorWords[2]]};
}

std::optional<std::vector<std::uint32_t>> parseModelPacketIdentityTargets(std::string_view text) {
  std::vector<std::uint32_t> targets;
  std::size_t cursor = 0;
  while (cursor < text.size()) {
    while (cursor < text.size() && separator(text[cursor])) {
      ++cursor;
    }
    if (cursor == text.size()) {
      break;
    }
    const std::size_t begin = cursor;
    while (cursor < text.size() && !separator(text[cursor])) {
      ++cursor;
    }
    std::string_view token = text.substr(begin, cursor - begin);
    if (token.starts_with("0x") || token.starts_with("0X")) {
      token.remove_prefix(2);
    }
    std::uint32_t target = 0;
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), target, 16);
    if (token.empty() || parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size()) {
      return std::nullopt;
    }
    targets.push_back(target);
  }
  return targets;
}

const char *modelFaceRejectionName(std::uint8_t rejection) {
  switch (static_cast<ModelFaceRejection>(rejection)) {
  case ModelFaceRejection::None:
    return "accepted";
  case ModelFaceRejection::ZeroUntexturedDepth:
    return "zero-depth";
  case ModelFaceRejection::FarDepth:
    return "far-depth";
  case ModelFaceRejection::Winding:
    return "winding";
  }
  return "unknown";
}

} // namespace crashbash::render
