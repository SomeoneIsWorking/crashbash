#include "model_packet_identity_diagnostic.h"

#include "config_var.h"
#include "core.h"
#include "crashbash_frame_driver.h"
#include "guest_execution.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <lucent/log.h>
#include <string>
#include <utility>

namespace crashbash::render {
namespace {

constexpr std::uint32_t kPacketGeometryFill = 0x800193A8u;
constexpr std::uint32_t kPacketRecordStride = 0x28u;

psx::config::TextVar cvPacketIdentityNodes(
    "PSXPORT_CRASHBASH_PACKET_NODES",
    "",
    "diagnostic: comma/space-separated guest OT packet tag addresses to bind to Crash Bash model source faces",
    /*persistable=*/false);

void packetGeometryFill(Core *core) {
  ModelPacketFillObservation observation{
      .packetBlock = core->r[4],
      .vertexBase = core->r[5],
      .topologyBase = core->r[6],
  };
  runtime::callOriginal(*core, runtime::GuestImage::Resident, kPacketGeometryFill);
  frameDriver(*core).packetIdentityDiagnostic().observeBlock(std::move(observation));
}

} // namespace

void PacketIdentityDiagnostic::beginFrame() {
  targets_.clear();
  matches_.clear();
  draws_ = 0;
  packetBlocks_ = 0;
  targetComparisons_ = 0;
  pendingDrawPacketBlocks_.clear();
  const std::string &setting = cvPacketIdentityNodes.get();
  const auto targets = parseModelPacketIdentityTargets(setting);
  if (!targets) {
    lucent::error("crashbash-packet-identity",
                  "PSXPORT_CRASHBASH_PACKET_NODES='{}' is invalid; expected comma/space-separated hexadecimal guest "
                  "addresses",
                  setting);
    std::abort();
  }
  targets_ = *targets;
}

void PacketIdentityDiagnostic::beginDraw() {
  if (!targets_.empty()) {
    pendingDrawPacketBlocks_.emplace_back();
  }
}

void PacketIdentityDiagnostic::observeBlock(ModelPacketFillObservation observation) {
  if (!pendingDrawPacketBlocks_.empty()) {
    pendingDrawPacketBlocks_.back().push_back(std::move(observation));
  }
}

void PacketIdentityDiagnostic::finishDraw(const ModelDraw &draw) {
  if (targets_.empty()) {
    return;
  }
  if (pendingDrawPacketBlocks_.empty()) {
    lucent::error("crashbash-packet-identity", "model packet diagnostic draw stack underflow");
    std::abort();
  }
  std::vector<ModelPacketFillObservation> packetBlocks = std::move(pendingDrawPacketBlocks_.back());
  pendingDrawPacketBlocks_.pop_back();
  ++draws_;
  for (const ModelPacketFillObservation &observation : packetBlocks) {
    for (const std::uint32_t target : targets_) {
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
  const ModelPacketIdentityScan scan = scanModelPacketIdentity(draw, packetBlocks, targets_);
  packetBlocks_ += scan.packetBlocks;
  targetComparisons_ += scan.targetComparisons;
  matches_.insert(matches_.end(), scan.matches.begin(), scan.matches.end());
}

void PacketIdentityDiagnostic::reportFrame(std::uint32_t frame) {
  if (targets_.empty()) {
    return;
  }
  lucent::debug("crashbash-packet-identity",
                "f{} targets={} draws={} packet-blocks={} comparisons={} matches={}",
                frame,
                targets_.size(),
                draws_,
                packetBlocks_,
                targetComparisons_,
                matches_.size());
  for (const ModelPacketIdentity &identity : matches_) {
    const ModelFace &face = identity.face;
    const ModelPacketGeometryComparison &geometry = identity.geometry;
    const std::array<std::uint32_t, 3> packetColors =
        identity.payload ? modelPacketColors(*identity.payload, face.textured) : std::array<std::uint32_t, 3>{};
    lucent::debug("crashbash-packet-identity",
                  "f{} packet={:08X} block={:08X} submitter={:08X} obj={:08X} asset={:08X} data={:08X} "
                  "frame={:04X} flags={:08X}/{:08X} cue={}:({},{},{}) face={} mat={:04X} topo={:02X} "
                  "textured={} tpage={:04X} clut={:04X} semi={} blend={} "
                  "raw={:08X}/{:08X}/{:08X} modeled={:08X}/{:08X}/{:08X} packet={:08X}/{:08X}/{:08X}",
                  frame,
                  identity.packetNode,
                  identity.packetBlock,
                  static_cast<std::uint32_t>(identity.submitter),
                  identity.object,
                  identity.modelAsset,
                  identity.modelData,
                  identity.frameCode,
                  identity.objectFlags,
                  identity.callFlags,
                  identity.depthCueFactor,
                  identity.depthCueFarColor[0],
                  identity.depthCueFarColor[1],
                  identity.depthCueFarColor[2],
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
                  face.colors[2],
                  face.retailColors[0],
                  face.retailColors[1],
                  face.retailColors[2],
                  packetColors[0],
                  packetColors[1],
                  packetColors[2]);
    if (geometry.valid) {
      lucent::debug("crashbash-packet-identity",
                    "f{} packet={:08X} live-vtx={:08X} live-topo={:08X} source-vtx={:08X} "
                    "group={}/{} source=({},{},{},{:04X})/({},{},{},{:04X})/({},{},{},{:04X}) "
                    "packet-sxy=({},{})/({},{})/({},{}) native-sxy=({},{})/({},{})/({},{}) "
                    "native-xyf=({:.6f},{:.6f})/({:.6f},{:.6f})/({:.6f},{:.6f}) "
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
                    geometry.nativeFloatVertices[0][0],
                    geometry.nativeFloatVertices[0][1],
                    geometry.nativeFloatVertices[1][0],
                    geometry.nativeFloatVertices[1][1],
                    geometry.nativeFloatVertices[2][0],
                    geometry.nativeFloatVertices[2][1],
                    geometry.nativeDepths[0],
                    geometry.nativeDepths[1],
                    geometry.nativeDepths[2],
                    geometry.depthScale,
                    geometry.nativeOtz,
                    geometry.projectedCoordinatesMatch,
                    modelFaceRejectionName(geometry.nativeRejection),
                    geometry.nativeSortKey);
    }
  }
}

void registerModelPacketIdentityDiagnosticOverride(Core &core) {
  runtime::registerNativeOverride(core,
                                  runtime::GuestImage::Resident,
                                  kPacketGeometryFill,
                                  "CrashBash::ModelPacketIdentityDiagnostic",
                                  packetGeometryFill);
}

} // namespace crashbash::render
