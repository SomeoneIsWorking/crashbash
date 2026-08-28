#include "model_packet_identity_diagnostic.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

void require(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
  }
}

crashbash::render::ModelDraw sampleDraw() {
  using namespace crashbash::render;
  ModelDraw draw{
      .submitter = ModelSubmitter::Alternate,
      .object = 0x80123450u,
      .modelAsset = 0x80110000u,
      .modelData = 0x80111000u,
      .frameCode = 0x200Bu,
  };
  draw.faces = {
      ModelFace{.sourceFace = 0u, .sourceMaterial = 0x1111u},
      ModelFace{
          .vertices = {{{.x = 0, .y = 0, .z = 512}, {.x = 32, .y = 0, .z = 512}, {.x = 0, .y = 32, .z = 512}}},
          .sourceFace = 1u,
          .sourceVertexAddress = 0x80101018u,
          .sourceGroup = 4u,
          .sourceGroupFace = 2u,
          .sourceMaterial = 0x0222u,
          .topologyFlags = 1u,
      },
      ModelFace{.sourceFace = 2u, .sourceMaterial = 0x0333u},
  };
  draw.transform.rotation[0][0] = 4096;
  draw.transform.rotation[1][1] = 4096;
  draw.transform.rotation[2][2] = 4096;
  draw.transform.projectionX = 160 << 16;
  draw.transform.projectionY = 120 << 16;
  draw.transform.projectionDistance = 256u;
  draw.transform.valid = true;
  draw.depthLimit = 32767;
  draw.depthScale = 341;
  return draw;
}

} // namespace

int main() {
  using namespace crashbash::render;
  const ModelDraw draw = sampleDraw();
  const std::uint32_t block = 0x800C2ACCu;

  const auto identity = identifyModelPacketNode(draw, block, block + 0x28u);
  require(identity.has_value(), "the second 0x28-byte packet record must map");
  require(identity->packetBlock == block, "the allocator block base must survive attribution");
  require(identity->object == draw.object && identity->frameCode == draw.frameCode,
          "the packet must retain its object/frame owner");
  require(identity->face.sourceFace == 1u && identity->face.sourceMaterial == 0x0222u,
          "the packet index must select the corresponding decoded source face/material");

  require(!identifyModelPacketNode(draw, block, block - 4u), "an address before the block must not map");
  require(!identifyModelPacketNode(draw, block, block + 4u), "a payload/misaligned address must not map as a tag");
  require(!identifyModelPacketNode(draw, block, block + 3u * 0x28u),
          "the first packet after the decoded face denominator must not map");

  ModelPacketPayload payload{.packetNode = block + 0x28u};
  payload.words[4] = 160u | (120u << 16u);
  payload.words[6] = 176u | (120u << 16u);
  payload.words[8] = 160u | (136u << 16u);
  const ModelPacketIdentityScan positiveScan =
      scanModelPacketIdentity(draw,
                              {ModelPacketFillObservation{
                                   .packetBlock = block,
                                   .vertexBase = 0x80101000u,
                                   .topologyBase = 0x80102000u,
                                   .payloads = {payload},
                               },
                               ModelPacketFillObservation{.packetBlock = 0x800D0000u}},
                              {block + 0x28u, 0x800E0000u});
  require(positiveScan.packetBlocks == 2u && positiveScan.targetComparisons == 4u && positiveScan.matches.size() == 1u,
          "the scan must report its complete positive denominator and sole match");
  const ModelPacketIdentity &observed = positiveScan.matches[0];
  require(observed.fillVertexBase == 0x80101000u && observed.fillTopologyBase == 0x80102000u,
          "the identity must retain the live packet-fill source bases");
  require(observed.geometry.valid && observed.geometry.projectedCoordinatesMatch,
          "the diagnostic must detect equal retail-packet and native projected coordinates");
  require(observed.geometry.nativeRejection == 0u,
          "the matching front-facing source face must remain accepted by native coverage");

  payload.words[8] += 1u;
  const ModelPacketIdentityScan mismatchScan = scanModelPacketIdentity(
      draw, {ModelPacketFillObservation{.packetBlock = block, .payloads = {payload}}}, {block + 0x28u});
  require(mismatchScan.matches.size() == 1u && mismatchScan.matches[0].geometry.valid &&
              !mismatchScan.matches[0].geometry.projectedCoordinatesMatch,
          "a corrupted packet coordinate must produce the instrument's opposite answer");

  const ModelPacketIdentityScan zeroMatchScan = scanModelPacketIdentity(
      draw,
      {ModelPacketFillObservation{.packetBlock = block}, ModelPacketFillObservation{.packetBlock = 0x800D0000u}},
      {0x800E0000u});
  require(zeroMatchScan.packetBlocks == 2u && zeroMatchScan.targetComparisons == 2u && zeroMatchScan.matches.empty(),
          "a zero-match scan must retain its nonzero block/comparison denominator");

  const auto targets = parseModelPacketIdentityTargets("0x800C2FF4, 800C8D84\t800BDABC");
  require(targets.has_value() && *targets == std::vector<std::uint32_t>{0x800C2FF4u, 0x800C8D84u, 0x800BDABCu},
          "the diagnostic must accept the runtime's printed packet-address spellings");
  require(!parseModelPacketIdentityTargets("800C2FF4,bad-node"), "an invalid target must be rejected, not ignored");

  std::puts("model packet identity diagnostic tests passed");
  return 0;
}
