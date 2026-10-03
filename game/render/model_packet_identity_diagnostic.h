#pragma once

#include "model_packet_identity.h"

#include <cstdint>
#include <vector>

class Core;

namespace crashbash::render {

// The per-frame packet-identity evidence: which guest OT packet tags were bound to which source
// face, and how the live packet geometry compared with the native projection of that face.
// Diagnostic only, and one instance belongs to the frame driver whose run it measures.
class PacketIdentityDiagnostic {
public:
  void beginFrame();
  void beginDraw();
  void observeBlock(ModelPacketFillObservation observation);
  void finishDraw(const ModelDraw &draw);
  void reportFrame(std::uint32_t frame);

private:
  std::vector<std::uint32_t> targets_;
  std::vector<ModelPacketIdentity> matches_;
  std::vector<std::vector<ModelPacketFillObservation>> pendingDrawPacketBlocks_;
  std::uint32_t draws_ = 0;
  std::uint32_t packetBlocks_ = 0;
  std::uint32_t targetComparisons_ = 0;
};

void registerModelPacketIdentityDiagnosticOverride(Core &core);

} // namespace crashbash::render
