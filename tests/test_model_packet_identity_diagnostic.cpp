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
      ModelFace{.sourceFace = 1u, .sourceMaterial = 0x8222u, .topologyFlags = 1u},
      ModelFace{.sourceFace = 2u, .sourceMaterial = 0x0333u},
  };
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
  require(identity->face.sourceFace == 1u && identity->face.sourceMaterial == 0x8222u,
          "the packet index must select the corresponding decoded source face/material");

  require(!identifyModelPacketNode(draw, block, block - 4u), "an address before the block must not map");
  require(!identifyModelPacketNode(draw, block, block + 4u), "a payload/misaligned address must not map as a tag");
  require(!identifyModelPacketNode(draw, block, block + 3u * 0x28u),
          "the first packet after the decoded face denominator must not map");

  const ModelPacketIdentityScan positiveScan =
      scanModelPacketIdentity(draw, {block, 0x800D0000u}, {block + 0x28u, 0x800E0000u});
  require(positiveScan.packetBlocks == 2u && positiveScan.targetComparisons == 4u && positiveScan.matches.size() == 1u,
          "the scan must report its complete positive denominator and sole match");
  const ModelPacketIdentityScan zeroMatchScan = scanModelPacketIdentity(draw, {block, 0x800D0000u}, {0x800E0000u});
  require(zeroMatchScan.packetBlocks == 2u && zeroMatchScan.targetComparisons == 2u && zeroMatchScan.matches.empty(),
          "a zero-match scan must retain its nonzero block/comparison denominator");

  const auto targets = parseModelPacketIdentityTargets("0x800C2FF4, 800C8D84\t800BDABC");
  require(targets.has_value() && *targets == std::vector<std::uint32_t>{0x800C2FF4u, 0x800C8D84u, 0x800BDABCu},
          "the diagnostic must accept the runtime's printed packet-address spellings");
  require(!parseModelPacketIdentityTargets("800C2FF4,bad-node"), "an invalid target must be rejected, not ignored");

  std::puts("model packet identity diagnostic tests passed");
  return 0;
}
