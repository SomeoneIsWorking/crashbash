#include "model_packet_identity_diagnostic.h"

#include "config_var.h"
#include "core.h"
#include "model_face_coverage.h"
#include "native_projection.h"
#include "override_registry.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <lucent/log.h>
#include <string_view>
#include <vector>

#ifdef CRASHBASH_HAVE_SUBSTRATE
#include "rec_decls.h"
#endif

namespace crashbash::render {
namespace {

constexpr std::uint32_t kPacketGeometryFill = 0x800193A8u;
constexpr std::uint32_t kPacketRecordStride = 0x28u;

psx::config::TextVar cvPacketIdentityNodes(
    "PSXPORT_CRASHBASH_PACKET_NODES",
    "",
    "diagnostic: comma/space-separated guest OT packet tag addresses to bind to Crash Bash model source faces",
    /*persistable=*/false);

struct ModelPacketIdentityFrameCensus {
  std::vector<std::uint32_t> targets;
  std::vector<ModelPacketIdentity> matches;
  std::uint32_t draws = 0;
  std::uint32_t packetBlocks = 0;
  std::uint32_t targetComparisons = 0;
};

thread_local ModelPacketIdentityFrameCensus census;
thread_local std::vector<std::vector<ModelPacketFillObservation>> pendingDrawPacketBlocks;

bool separator(char value) {
  return value == ',' || value == ' ' || value == '\t' || value == '\n';
}

#ifdef CRASHBASH_HAVE_SUBSTRATE
void packetGeometryFill(Core *core) {
  ModelPacketFillObservation observation{
      .packetBlock = core->r[4],
      .vertexBase = core->r[5],
      .topologyBase = core->r[6],
  };
  gen_func_800193A8(core);
  for (const std::uint32_t target : census.targets) {
    if (target < 0x80000000u || target > 0x80200000u - kPacketRecordStride) {
      continue;
    }
    ModelPacketPayload payload{.packetNode = target};
    for (std::uint32_t word = 0; word < payload.words.size(); ++word) {
      payload.words[word] = core->mem_r32(target + word * 4u);
    }
    observation.payloads.push_back(payload);
  }
  observeModelPacketBlock(std::move(observation));
}
#endif

std::array<std::int16_t, 2> unpackSxy(std::uint32_t word) {
  return {
      static_cast<std::int16_t>(word),
      static_cast<std::int16_t>(word >> 16u),
  };
}

const char *rejectionName(ModelFaceRejection rejection) {
  switch (rejection) {
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
      .modelAsset = draw.modelAsset,
      .modelData = draw.modelData,
      .frameCode = draw.frameCode,
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

void beginModelPacketIdentityDiagnosticFrame() {
  census = {};
  pendingDrawPacketBlocks.clear();
  const std::string &setting = cvPacketIdentityNodes.get();
  const auto targets = parseModelPacketIdentityTargets(setting);
  if (!targets) {
    lucent::error("crashbash-packet-identity",
                  "PSXPORT_CRASHBASH_PACKET_NODES='{}' is invalid; expected comma/space-separated hexadecimal guest "
                  "addresses",
                  setting);
    std::abort();
  }
  census.targets = *targets;
}

void beginModelPacketIdentityDraw() {
  if (!census.targets.empty()) {
    pendingDrawPacketBlocks.emplace_back();
  }
}

void observeModelPacketBlock(ModelPacketFillObservation observation) {
  if (!pendingDrawPacketBlocks.empty()) {
    pendingDrawPacketBlocks.back().push_back(std::move(observation));
  }
}

void finishModelPacketIdentityDraw(const ModelDraw &draw) {
  if (census.targets.empty()) {
    return;
  }
  if (pendingDrawPacketBlocks.empty()) {
    lucent::error("crashbash-packet-identity", "model packet diagnostic draw stack underflow");
    std::abort();
  }
  std::vector<ModelPacketFillObservation> packetBlocks = std::move(pendingDrawPacketBlocks.back());
  pendingDrawPacketBlocks.pop_back();
  ++census.draws;
  for (const ModelPacketFillObservation &observation : packetBlocks) {
    for (const std::uint32_t target : census.targets) {
      if (target < observation.packetBlock) {
        continue;
      }
      const std::uint32_t offset = target - observation.packetBlock;
      if (offset % kPacketRecordStride != 0) {
        continue;
      }
      const std::uint32_t faceIndex = offset / kPacketRecordStride;
      if (faceIndex >= draw.faces.size()) {
        lucent::debug("crashbash-packet-identity",
                      "target={:08X} candidate block={:08X} face={} decoded-faces={} submitter={:08X} obj={:08X} "
                      "flags={:08X}/{:08X} asset={:08X} data={:08X} frame={:04X} live-vtx={:08X} "
                      "live-topo={:08X}",
                      target,
                      observation.packetBlock,
                      faceIndex,
                      draw.faces.size(),
                      static_cast<std::uint32_t>(draw.submitter),
                      draw.object,
                      draw.objectFlags,
                      draw.callFlags,
                      draw.modelAsset,
                      draw.modelData,
                      draw.frameCode,
                      observation.vertexBase,
                      observation.topologyBase);
      }
    }
  }
  const ModelPacketIdentityScan scan = scanModelPacketIdentity(draw, packetBlocks, census.targets);
  census.packetBlocks += scan.packetBlocks;
  census.targetComparisons += scan.targetComparisons;
  census.matches.insert(census.matches.end(), scan.matches.begin(), scan.matches.end());
}

void reportModelPacketIdentityDiagnosticFrame(std::uint32_t frame) {
  if (census.targets.empty()) {
    return;
  }
  lucent::debug("crashbash-packet-identity",
                "f{} targets={} draws={} packet-blocks={} comparisons={} matches={}",
                frame,
                census.targets.size(),
                census.draws,
                census.packetBlocks,
                census.targetComparisons,
                census.matches.size());
  for (const ModelPacketIdentity &identity : census.matches) {
    const ModelFace &face = identity.face;
    const ModelPacketGeometryComparison &geometry = identity.geometry;
    lucent::debug("crashbash-packet-identity",
                  "f{} packet={:08X} block={:08X} submitter={:08X} obj={:08X} asset={:08X} data={:08X} "
                  "frame={:04X} face={} mat={:04X} topo={:02X} textured={} tpage={:04X} clut={:04X} "
                  "semi={} blend={} colors={:08X}/{:08X}/{:08X}",
                  frame,
                  identity.packetNode,
                  identity.packetBlock,
                  static_cast<std::uint32_t>(identity.submitter),
                  identity.object,
                  identity.modelAsset,
                  identity.modelData,
                  identity.frameCode,
                  face.sourceFace,
                  face.sourceMaterial,
                  face.topologyFlags,
                  face.textured,
                  face.texturePage,
                  face.clut,
                  face.semiTransparent,
                  face.blendMode,
                  face.colors[0],
                  face.colors[1],
                  face.colors[2]);
    if (geometry.valid) {
      lucent::debug("crashbash-packet-identity",
                    "f{} packet={:08X} live-vtx={:08X} live-topo={:08X} source-vtx={:08X} "
                    "group={}/{} source=({},{},{},{:04X})/({},{},{},{:04X})/({},{},{},{:04X}) "
                    "packet-sxy=({},{})/({},{})/({},{}) native-sxy=({},{})/({},{})/({},{}) "
                    "native-sz={}/{}/{} zsf3={} otz={} projection-match={} coverage={} sort={}",
                    frame,
                    identity.packetNode,
                    identity.fillVertexBase,
                    identity.fillTopologyBase,
                    face.sourceVertexAddress,
                    face.sourceGroup,
                    face.sourceGroupFace,
                    face.vertices[0].x,
                    face.vertices[0].y,
                    face.vertices[0].z,
                    face.vertices[0].flags,
                    face.vertices[1].x,
                    face.vertices[1].y,
                    face.vertices[1].z,
                    face.vertices[1].flags,
                    face.vertices[2].x,
                    face.vertices[2].y,
                    face.vertices[2].z,
                    face.vertices[2].flags,
                    geometry.packetVertices[0][0],
                    geometry.packetVertices[0][1],
                    geometry.packetVertices[1][0],
                    geometry.packetVertices[1][1],
                    geometry.packetVertices[2][0],
                    geometry.packetVertices[2][1],
                    geometry.nativeVertices[0][0],
                    geometry.nativeVertices[0][1],
                    geometry.nativeVertices[1][0],
                    geometry.nativeVertices[1][1],
                    geometry.nativeVertices[2][0],
                    geometry.nativeVertices[2][1],
                    geometry.nativeDepths[0],
                    geometry.nativeDepths[1],
                    geometry.nativeDepths[2],
                    geometry.depthScale,
                    geometry.nativeOtz,
                    geometry.projectedCoordinatesMatch,
                    rejectionName(static_cast<ModelFaceRejection>(geometry.nativeRejection)),
                    geometry.nativeSortKey);
    }
  }
}

void registerModelPacketIdentityDiagnosticOverride() {
#ifdef CRASHBASH_HAVE_SUBSTRATE
  overrides::install(kPacketGeometryFill,
                     "CrashBash::ModelPacketIdentityDiagnostic",
                     packetGeometryFill,
                     gen_func_800193A8,
                     shard_set_override);
#endif
}

} // namespace crashbash::render
