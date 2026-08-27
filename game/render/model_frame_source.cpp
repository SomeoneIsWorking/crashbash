#include "model_frame_source.h"

#include <cstdint>
#include <optional>

namespace crashbash::render {
namespace {

constexpr std::uint32_t kFrameFamilyMask = 0x7000u;
constexpr std::uint32_t kDirectFrameFamily = 0x2000u;
constexpr std::uint32_t kResolvedFrameFamily = 0x5000u;
constexpr std::uint32_t kFrameIndexMask = 0x0FFFu;
constexpr std::uint32_t kFrameRecordOffset = 0x24u;
constexpr std::uint32_t kFrameRecordStride = 0x34u;

std::optional<std::uint32_t>
resolveDirectFrame(std::uint16_t frameCode, std::uint32_t modelData, const ModelFrameWordReader &readWord) {
  const std::uint32_t frameIndex = frameCode & kFrameIndexMask;
  const std::optional<std::uint32_t> lastFrame = readWord(modelData + 0x54u);
  if (!lastFrame || static_cast<std::int32_t>(*lastFrame) < static_cast<std::int32_t>(frameIndex)) {
    return std::nullopt;
  }
  return modelData + kFrameRecordOffset + frameIndex * kFrameRecordStride;
}

std::optional<std::uint32_t>
resolveIndirectFrame(std::uint16_t frameCode, std::uint32_t modelData, const ModelFrameWordReader &readWord) {
  const std::optional<std::uint32_t> tableRelative = readWord(modelData + 0x1Cu);
  if (!tableRelative) {
    return std::nullopt;
  }

  const std::uint32_t frameIndex = (static_cast<std::uint32_t>(frameCode) - 1u) & kFrameIndexMask;
  const std::uint32_t descriptor = modelData + *tableRelative + 0x20u + frameIndex * 0x0Cu;
  const std::optional<std::uint32_t> lookupRelative = readWord(descriptor + 4u);
  if (!lookupRelative) {
    return std::nullopt;
  }
  const std::optional<std::uint32_t> frameRelative = readWord(descriptor + *lookupRelative + 4u);
  if (!frameRelative || *frameRelative == 0u) {
    return std::nullopt;
  }
  const std::optional<std::uint32_t> frameBase = readWord(descriptor);
  if (!frameBase) {
    return std::nullopt;
  }
  return *frameBase + *frameRelative;
}

} // namespace

std::optional<std::uint32_t>
resolveModelFrameSource(std::uint16_t frameCode, std::uint32_t modelData, const ModelFrameWordReader &readWord) {
  switch (frameCode & kFrameFamilyMask) {
  case kDirectFrameFamily:
    return resolveDirectFrame(frameCode, modelData, readWord);
  case kResolvedFrameFamily:
    return resolveIndirectFrame(frameCode, modelData, readWord);
  default:
    return std::nullopt;
  }
}

} // namespace crashbash::render
