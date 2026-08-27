#include "model_transform_capture.h"

#include "core.h"
#include "override_registry.h"

#include <cstdint>

#ifdef CRASHBASH_HAVE_SUBSTRATE
#include "rec_decls.h"
#endif

namespace crashbash::render {
namespace {

constexpr std::uint32_t kModelTransformComposer = 0x8001965Cu;
constexpr std::uint32_t kProjectionGlobalsPointer = 0x8005B698u;
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
  const std::uint32_t projectionGlobals = core->mem_r32(kProjectionGlobalsPointer);
  census.lastRotation = rotation;
  census.lastTranslation = translation;
  census.lastProjection = projectionGlobals;
  if (!guestRange(rotation, 18u)) {
    ++census.invalidRotation;
    return;
  }
  if (translation != 0 && !guestRange(translation, 12u)) {
    ++census.missingTranslation;
    return;
  }
  if (!guestRange(projectionGlobals, 6u)) {
    ++census.invalidProjection;
    return;
  }

  ModelTransform transform{};
  for (std::uint32_t row = 0; row < 3; ++row) {
    for (std::uint32_t column = 0; column < 3; ++column) {
      transform.rotation[row][column] = static_cast<std::int16_t>(core->mem_r16(rotation + (row * 3u + column) * 2u));
    }
    transform.translation[row] =
        translation == 0 ? 0 : static_cast<std::int32_t>(core->mem_r32(translation + row * 4u));
  }
  const std::int16_t centerX = static_cast<std::int16_t>(core->mem_r16(output + 8u));
  const std::int16_t centerY = static_cast<std::int16_t>(core->mem_r16(output + 10u));
  transform.projectionDistance = core->mem_r16(projectionGlobals + 4u);
  transform.projectionX =
      static_cast<std::int32_t>((static_cast<std::int64_t>(centerX) * transform.projectionDistance / 0x280) << 16);
  transform.projectionY = static_cast<std::int32_t>(signedHalf(centerY)) << 16;
  transform.valid = transform.projectionDistance != 0;
  pending.transform = transform;
  census.captured += transform.valid ? 1u : 0u;
}
#endif

} // namespace

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
#endif
}

} // namespace crashbash::render
