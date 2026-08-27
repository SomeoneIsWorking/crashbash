#include "model_transform_capture.h"

#include "core.h"
#include "override_registry.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

#ifdef CRASHBASH_HAVE_SUBSTRATE
#include "rec_decls.h"
#endif

namespace crashbash::render {
namespace {

constexpr std::uint32_t kModelTransformComposer = 0x8001965Cu;
constexpr std::uint32_t kAlternateModelTransformComposer = 0x8001D894u;
constexpr std::uint32_t kHorizontalProjectionScalePointer = 0x8005B698u;
constexpr std::uint32_t kCameraGlobalsPointer = 0x800569E0u;
constexpr std::uint32_t kAlternateViewRotation = 0x80058AD4u;
constexpr std::uint32_t kProjectionCenterX = 0x800569B8u;
constexpr std::uint32_t kProjectionCenterY = 0x800569BCu;
constexpr std::uint32_t kRamBegin = 0x80000000u;
constexpr std::uint32_t kRamEnd = 0x80200000u;
constexpr std::uint32_t kScratchpadBegin = 0x1F800000u;
constexpr std::uint32_t kScratchpadEnd = 0x1F800400u;

struct PendingTransform {
  Core *core = nullptr;
  std::uint32_t object = 0;
  ModelTransform transform;
};

thread_local PendingTransform pending;
thread_local ModelTransformCaptureCensus census;

bool guestRange(std::uint32_t address, std::uint32_t size) {
  const bool ram = address >= kRamBegin && address <= kRamEnd && size <= kRamEnd - address;
  const bool scratchpad = address >= kScratchpadBegin && address <= kScratchpadEnd && size <= kScratchpadEnd - address;
  return ram || scratchpad;
}

std::int16_t signedHalf(std::int16_t value) {
  return static_cast<std::int16_t>((static_cast<std::int32_t>(value) - (value < 0 ? -1 : 0)) >> 1);
}

std::int32_t clampIr(std::int64_t value) {
  return static_cast<std::int32_t>(std::clamp(value,
                                              static_cast<std::int64_t>(std::numeric_limits<std::int16_t>::min()),
                                              static_cast<std::int64_t>(std::numeric_limits<std::int16_t>::max())));
}

std::int64_t fixedShift(std::int64_t value) {
  return value >= 0 ? value >> 12u : -(((-value) + 0xFFF) >> 12u);
}

std::int32_t wrapAdd(std::int32_t left, std::int32_t right) {
  return static_cast<std::int32_t>(static_cast<std::uint32_t>(left) + static_cast<std::uint32_t>(right));
}

std::int32_t wrapSubtract(std::uint32_t left, std::uint32_t right) {
  return static_cast<std::int32_t>(left - right);
}

ModelRotation readRotation(Core &core, std::uint32_t address) {
  ModelRotation rotation{};
  for (std::uint32_t row = 0; row < rotation.size(); ++row) {
    for (std::uint32_t column = 0; column < rotation[row].size(); ++column) {
      rotation[row][column] = static_cast<std::int16_t>(core.mem_r16(address + (row * 3u + column) * 2u));
    }
  }
  return rotation;
}

#ifdef CRASHBASH_HAVE_SUBSTRATE
void modelTransformComposer(Core *core) {
  ++census.attempts;
  const std::uint32_t object = core->r[4];
  const std::uint32_t output = core->mem_r32(core->r[29] + 0x10u);
  gen_func_8001965C(core);

  census.lastOutput = output;
  if (pending.core != core || pending.object != object) {
    ++census.pendingMismatch;
    return;
  }
  if (!guestRange(output, 12u)) {
    ++census.invalidOutput;
    return;
  }
  const std::uint32_t rotation = core->mem_r32(output);
  const std::uint32_t translation = core->mem_r32(output + 4u);
  const std::uint32_t camera = core->mem_r32(kCameraGlobalsPointer);
  const std::uint32_t horizontalProjectionScale = core->mem_r32(kHorizontalProjectionScalePointer);
  census.lastRotation = rotation;
  census.lastTranslation = translation;
  census.lastCamera = camera;
  census.lastProjection = horizontalProjectionScale;
  if (!guestRange(rotation, 18u)) {
    ++census.invalidRotation;
    return;
  }
  if (translation != 0 && !guestRange(translation, 12u)) {
    ++census.missingTranslation;
    return;
  }
  if (!guestRange(camera, 0x1Cu)) {
    ++census.invalidCamera;
    return;
  }
  if (!guestRange(horizontalProjectionScale, 6u)) {
    ++census.invalidProjection;
    return;
  }

  ModelTransform transform{};
  transform.rotation = readRotation(*core, rotation);
  for (std::uint32_t row = 0; row < 3; ++row) {
    transform.translation[row] =
        translation == 0 ? 0 : static_cast<std::int32_t>(core->mem_r32(translation + row * 4u));
  }
  const std::int16_t centerX = static_cast<std::int16_t>(core->mem_r16(output + 8u));
  const std::int16_t centerY = static_cast<std::int16_t>(core->mem_r16(output + 10u));
  const std::int16_t horizontalScale = static_cast<std::int16_t>(core->mem_r16(horizontalProjectionScale + 4u));
  transform.projectionDistance = static_cast<std::uint16_t>(core->mem_r32(camera + 0x18u));
  transform.projectionX =
      static_cast<std::int32_t>((static_cast<std::int64_t>(centerX) * horizontalScale / 0x280) << 16);
  transform.projectionY = static_cast<std::int32_t>(signedHalf(centerY)) << 16;
  transform.valid = transform.projectionDistance != 0;
  pending.transform = transform;
  census.captured += transform.valid ? 1u : 0u;
}

void alternateModelTransformComposer(Core *core) {
  ++census.attempts;
  const std::uint32_t object = core->r[4];
  const std::uint32_t callFlags = core->r[6];
  gen_func_8001D894(core);

  if (pending.core != core || pending.object != object) {
    ++census.pendingMismatch;
    return;
  }
  const std::uint32_t camera = core->mem_r32(kCameraGlobalsPointer);
  const std::uint32_t horizontalProjectionScale = core->mem_r32(kHorizontalProjectionScalePointer);
  const std::uint32_t viewRotation = (callFlags & 0x00200000u) == 0 ? camera + 0x74u : kAlternateViewRotation;
  census.lastOutput = object;
  census.lastRotation = viewRotation;
  census.lastTranslation = camera;
  census.lastCamera = camera;
  census.lastProjection = horizontalProjectionScale;
  if (!guestRange(object, 0x6Cu) || !guestRange(camera, 0xAAu) || !guestRange(viewRotation, 18u) ||
      !guestRange(horizontalProjectionScale, 6u)) {
    ++census.invalidOutput;
    return;
  }

  const ModelRotation view = readRotation(*core, viewRotation);
  ModelTransform transform{};
  if ((core->mem_r32(object) & 0x20000000u) == 0) {
    transform.rotation = composeModelRotations(view, readRotation(*core, object + 0x30u));
  } else {
    transform.rotation = readRotation(*core, camera + 0x98u);
  }
  const std::array<std::int32_t, 3> relative{
      wrapSubtract(core->mem_r32(object + 4u), core->mem_r32(camera + 0x0Cu)),
      wrapSubtract(core->mem_r32(object + 8u), core->mem_r32(camera + 0x10u)),
      wrapSubtract(core->mem_r32(object + 0x0Cu), core->mem_r32(camera + 0x14u)),
  };
  transform.translation = transformLargeModelTranslation(view, relative);
  for (std::uint32_t component = 0; component < transform.translation.size(); ++component) {
    transform.translation[component] = wrapAdd(
        transform.translation[component], static_cast<std::int32_t>(core->mem_r32(camera + 0x88u + component * 4u)));
  }
  const auto horizontalScale = static_cast<std::int16_t>(core->mem_r16(horizontalProjectionScale + 4u));
  transform.projectionDistance = static_cast<std::uint16_t>(core->mem_r32(camera + 0x18u));
  const auto centerX = static_cast<std::int32_t>(core->mem_r32(kProjectionCenterX));
  const auto centerY = static_cast<std::int32_t>(core->mem_r32(kProjectionCenterY));
  transform.projectionX = static_cast<std::int32_t>(
      static_cast<std::uint32_t>((static_cast<std::int64_t>(centerX) * horizontalScale) / 0x280) << 16u);
  transform.projectionY = static_cast<std::int32_t>(static_cast<std::uint32_t>(centerY / 2) << 16u);
  transform.valid = transform.projectionDistance != 0;
  pending.transform = transform;
  census.captured += transform.valid ? 1u : 0u;
}
#endif

} // namespace

ModelRotation composeModelRotations(const ModelRotation &left, const ModelRotation &right) {
  ModelRotation result{};
  for (std::uint32_t row = 0; row < result.size(); ++row) {
    for (std::uint32_t column = 0; column < result[row].size(); ++column) {
      std::int64_t accumulator = 0;
      for (std::uint32_t inner = 0; inner < result.size(); ++inner) {
        accumulator += static_cast<std::int64_t>(left[row][inner]) * right[inner][column];
      }
      result[row][column] = static_cast<std::int16_t>(clampIr(fixedShift(accumulator)));
    }
  }
  return result;
}

std::array<std::int32_t, 3> transformLargeModelTranslation(const ModelRotation &rotation,
                                                           const std::array<std::int32_t, 3> &translation) {
  std::array<std::int32_t, 3> high{};
  std::array<std::int32_t, 3> low{};
  for (std::uint32_t component = 0; component < translation.size(); ++component) {
    const std::int64_t value = translation[component];
    const std::int32_t quotient = static_cast<std::int32_t>(value >= 0 ? value >> 15u : -((-value) >> 15u));
    high[component] = static_cast<std::int16_t>(quotient);
    low[component] = static_cast<std::int32_t>(value >= 0 ? value & 0x7FFF : -((-value) & 0x7FFF));
  }

  std::array<std::int32_t, 3> result{};
  for (std::uint32_t row = 0; row < result.size(); ++row) {
    std::int64_t highAccumulator = 0;
    std::int64_t lowAccumulator = 0;
    for (std::uint32_t column = 0; column < result.size(); ++column) {
      highAccumulator += static_cast<std::int64_t>(rotation[row][column]) * high[column];
      lowAccumulator += static_cast<std::int64_t>(rotation[row][column]) * low[column];
    }
    result[row] = clampIr(highAccumulator) * 8 + clampIr(fixedShift(lowAccumulator));
  }
  return result;
}

void resetModelTransformCapture(Core &core, std::uint32_t object) {
  pending = PendingTransform{.core = &core, .object = object};
}

bool takeModelTransformCapture(Core &core, std::uint32_t object, ModelTransform &out) {
  if (pending.core != &core || pending.object != object || !pending.transform.valid) {
    return false;
  }
  out = pending.transform;
  pending = {};
  return true;
}

const ModelTransformCaptureCensus &modelTransformCaptureCensus() {
  return census;
}

void registerModelTransformCaptureOverride() {
#ifdef CRASHBASH_HAVE_SUBSTRATE
  overrides::install(kModelTransformComposer,
                     "CrashBash::ModelTransformCapture",
                     modelTransformComposer,
                     gen_func_8001965C,
                     shard_set_override);
  overrides::install(kAlternateModelTransformComposer,
                     "CrashBash::AlternateModelTransformCapture",
                     alternateModelTransformComposer,
                     gen_func_8001D894,
                     shard_set_override);
#endif
}

} // namespace crashbash::render
