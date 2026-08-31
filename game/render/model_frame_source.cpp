#include "model_frame_source.h"

#include <cstdint>
#include <optional>

namespace crashbash::render {
namespace {

constexpr std::uint32_t kFrameFamilyMask = 0x7000u;
constexpr std::uint32_t kIndexedFrameFamilyA = 0x1000u;
constexpr std::uint32_t kDirectFrameFamily = 0x2000u;
constexpr std::uint32_t kIndexedFrameFamilyB = 0x4000u;
constexpr std::uint32_t kResolvedFrameFamily = 0x5000u;
constexpr std::uint32_t kFrameIndexMask = 0x0FFFu;
constexpr std::uint32_t kFrameRecordOffset = 0x24u;
constexpr std::uint32_t kFrameRecordStride = 0x34u;
constexpr std::uint32_t kAnimationGroupIndexMask = 0x0F80u;
constexpr std::uint32_t kAnimationFrameIndexMask = 0x007Fu;
constexpr std::uint32_t kAnimationFrameOverride = 0x00400000u;

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

std::optional<ModelFrameSource> resolveIndexedFrame(const ModelFrameResolveInputs &inputs,
                                                    const ModelFrameWordReader &readWord) {
  const std::uint32_t groupIndex = (inputs.frameCode & kAnimationGroupIndexMask) >> 7u;
  const std::optional<std::uint32_t> groupCount = readWord(inputs.modelData + 0x40u);
  const std::optional<std::uint32_t> groupTableRelative = readWord(inputs.modelData + 0x44u);
  if (!groupCount || !groupTableRelative || groupIndex >= *groupCount) {
    return std::nullopt;
  }

  const std::uint32_t descriptor = inputs.modelData + *groupTableRelative + 0x44u + groupIndex * 0x18u;
  const std::optional<std::uint32_t> animationHandle = readWord(descriptor + 0x14u);
  const std::optional<std::uint32_t> frameCount = readWord(descriptor + 8u);
  const std::optional<std::uint32_t> frameRelative = readWord(descriptor + 0x0Cu);
  if (!animationHandle || !frameCount || !frameRelative || *animationHandle == 0u) {
    return std::nullopt;
  }
  const std::optional<std::uint32_t> animation = readWord(*animationHandle);
  if (!animation || *animation == 0u) {
    return std::nullopt;
  }

  const std::int32_t frameIndex = static_cast<std::int32_t>(inputs.effectiveFlags) < 0
                                      ? inputs.objectAnimationFrame
                                      : inputs.frameCode & kAnimationFrameIndexMask;
  if (frameIndex < 0 || static_cast<std::uint32_t>(frameIndex) >= *frameCount) {
    return std::nullopt;
  }

  const std::uint32_t animationEntry = *animation + static_cast<std::uint32_t>(frameIndex) * 0x10u + 4u;
  const std::optional<std::uint32_t> vertexIndicesRelative = readWord(animationEntry);
  const std::optional<std::uint32_t> interpolationRelative = readWord(animationEntry + 4u);
  const std::optional<std::uint32_t> interpolationWeight = readWord(animationEntry + 8u);
  if (!vertexIndicesRelative || *vertexIndicesRelative == 0u || !interpolationWeight) {
    return std::nullopt;
  }

  std::uint32_t interpolationIndexStream = 0;
  std::uint32_t effectiveInterpolationWeight = *interpolationWeight;
  if (static_cast<std::int32_t>(inputs.effectiveFlags) < 0 && inputs.objectInterpolationWeight != 0u) {
    const std::uint32_t nextAnimationEntry = animationEntry + 0x10u;
    const std::optional<std::uint32_t> nextInterpolationRelative = readWord(nextAnimationEntry + 4u);
    const std::optional<std::uint32_t> nextVertexIndicesRelative = readWord(nextAnimationEntry);
    const std::optional<std::uint32_t> nextInterpolationWeight = readWord(nextAnimationEntry + 8u);
    if (!nextInterpolationRelative || !nextVertexIndicesRelative || !nextInterpolationWeight) {
      return std::nullopt;
    }
    const std::uint32_t targetWeight = *nextInterpolationWeight == 0u ? 0x1000u : *nextInterpolationWeight;
    const std::int64_t delta = static_cast<std::int64_t>(targetWeight) - *interpolationWeight;
    effectiveInterpolationWeight =
        static_cast<std::uint32_t>(static_cast<std::int64_t>(*interpolationWeight) +
                                   (static_cast<std::int64_t>(inputs.objectInterpolationWeight) * delta >> 16u));
    const std::uint32_t secondaryRelative =
        *nextInterpolationRelative == 0u ? *nextVertexIndicesRelative : *nextInterpolationRelative;
    interpolationIndexStream = nextAnimationEntry + secondaryRelative + 0x14u;
  } else if (*interpolationWeight != 0u) {
    if (!interpolationRelative || *interpolationRelative == 0u) {
      return std::nullopt;
    }
    interpolationIndexStream = animationEntry + *interpolationRelative + 0x18u;
  }

  std::uint32_t frameRecord = descriptor + *frameRelative + 0x0Cu;
  if ((inputs.effectiveFlags & kAnimationFrameOverride) != 0u) {
    frameRecord = inputs.modelData + kFrameRecordOffset +
                  static_cast<std::uint32_t>(inputs.objectFrameIndex & kFrameIndexMask) * kFrameRecordStride;
  }

  const std::optional<std::uint32_t> vertexPoolRelative = readWord(*animation);
  if (!vertexPoolRelative) {
    return std::nullopt;
  }
  std::uint32_t vertexPool = *animation + *vertexPoolRelative;
  if (*vertexPoolRelative == 0u) {
    const std::optional<std::uint32_t> fallbackVertexPoolRelative = readWord(inputs.modelData + 0x28u);
    if (!fallbackVertexPoolRelative) {
      return std::nullopt;
    }
    vertexPool = inputs.modelData + *fallbackVertexPoolRelative + 0x28u;
  }
  return ModelFrameSource{
      .frameRecord = frameRecord,
      .vertexIndexStream = animationEntry + *vertexIndicesRelative + 0x14u,
      .interpolationIndexStream = interpolationIndexStream,
      .vertexPool = vertexPool,
      .interpolationWeight = effectiveInterpolationWeight,
  };
}

} // namespace

bool modelFrameFamilySupported(std::uint16_t frameCode) {
  switch (frameCode & kFrameFamilyMask) {
  case kIndexedFrameFamilyA:
  case kDirectFrameFamily:
  case kIndexedFrameFamilyB:
  case kResolvedFrameFamily:
    return true;
  default:
    return false;
  }
}

std::optional<ModelFrameSource> resolveModelFrameSource(const ModelFrameResolveInputs &inputs,
                                                        const ModelFrameWordReader &readWord) {
  switch (inputs.frameCode & kFrameFamilyMask) {
  case kIndexedFrameFamilyA:
  case kIndexedFrameFamilyB:
    return resolveIndexedFrame(inputs, readWord);
  case kDirectFrameFamily: {
    const auto frame = resolveDirectFrame(inputs.frameCode, inputs.modelData, readWord);
    return frame ? std::optional<ModelFrameSource>(ModelFrameSource{.frameRecord = *frame}) : std::nullopt;
  }
  case kResolvedFrameFamily: {
    const auto frame = resolveIndirectFrame(inputs.frameCode, inputs.modelData, readWord);
    return frame ? std::optional<ModelFrameSource>(ModelFrameSource{.frameRecord = *frame}) : std::nullopt;
  }
  default:
    return std::nullopt;
  }
}

} // namespace crashbash::render
