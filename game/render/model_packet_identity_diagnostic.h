#pragma once

#include "scene_snapshot.h"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace crashbash::render {

struct ModelPacketIdentity {
  std::uint32_t packetNode = 0;
  std::uint32_t packetBlock = 0;
  std::uint32_t object = 0;
  std::uint32_t modelAsset = 0;
  std::uint32_t modelData = 0;
  std::uint16_t frameCode = 0;
  ModelSubmitter submitter = ModelSubmitter::Standard;
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
                                                const std::vector<std::uint32_t> &packetBlocks,
                                                const std::vector<std::uint32_t> &packetNodes);

std::optional<std::vector<std::uint32_t>> parseModelPacketIdentityTargets(std::string_view text);

void beginModelPacketIdentityDiagnosticFrame();
void beginModelPacketIdentityDraw();
void observeModelPacketBlock(std::uint32_t packetBlock);
void finishModelPacketIdentityDraw(const ModelDraw &draw);
void reportModelPacketIdentityDiagnosticFrame(std::uint32_t frame);
void registerModelPacketIdentityDiagnosticOverride();

} // namespace crashbash::render
