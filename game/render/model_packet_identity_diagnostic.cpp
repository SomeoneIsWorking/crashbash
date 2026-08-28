#include "model_packet_identity_diagnostic.h"

#include "config_var.h"
#include "core.h"
#include "override_registry.h"

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
thread_local std::vector<std::vector<std::uint32_t>> pendingDrawPacketBlocks;

bool separator(char value) {
  return value == ',' || value == ' ' || value == '\t' || value == '\n';
}

#ifdef CRASHBASH_HAVE_SUBSTRATE
void packetGeometryFill(Core *core) {
  observeModelPacketBlock(core->r[4]);
  gen_func_800193A8(core);
}
#endif

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
                                                const std::vector<std::uint32_t> &packetBlocks,
                                                const std::vector<std::uint32_t> &packetNodes) {
  ModelPacketIdentityScan scan{
      .packetBlocks = static_cast<std::uint32_t>(packetBlocks.size()),
      .targetComparisons = static_cast<std::uint32_t>(packetBlocks.size() * packetNodes.size()),
  };
  for (const std::uint32_t packetBlock : packetBlocks) {
    for (const std::uint32_t packetNode : packetNodes) {
      if (const auto identity = identifyModelPacketNode(draw, packetBlock, packetNode)) {
        scan.matches.push_back(*identity);
      }
    }
  }
  return scan;
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

void observeModelPacketBlock(std::uint32_t packetBlock) {
  if (!pendingDrawPacketBlocks.empty()) {
    pendingDrawPacketBlocks.back().push_back(packetBlock);
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
  std::vector<std::uint32_t> packetBlocks = std::move(pendingDrawPacketBlocks.back());
  pendingDrawPacketBlocks.pop_back();
  ++census.draws;
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
