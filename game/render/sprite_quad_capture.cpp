#include "sprite_quad_capture.h"

#include "core.h"
#include "crashbash_frame_driver.h"
#include "override_registry.h"
#include "sprite_quad_decode.h"

#include <cstdint>
#include <lucent/log.h>

#ifdef CRASHBASH_HAVE_SUBSTRATE
#include "rec_decls.h"
#endif

namespace crashbash::render {
namespace {

constexpr std::uint32_t kSpriteQuadSubmit = 0x8002992Cu;
constexpr std::uint32_t kFlatSpriteQuadSubmit = 0x80029D28u;
constexpr std::uint32_t kDisplayDescriptor = 0x8005B698u;
constexpr std::uint32_t kGlobalFade = 0x800569ACu;
constexpr std::uint32_t kSpriteRenderList = 0x800569D8u;

#ifdef CRASHBASH_HAVE_SUBSTRATE
SpriteQuadDescriptor readDescriptor(Core &core, std::uint32_t address) {
  return SpriteQuadDescriptor{
      .width = core.mem_r16(address + 0x08u),
      .height = core.mem_r16(address + 0x0Au),
      .texturePage = core.mem_r16(address + 0x24u),
      .clut = core.mem_r16(address + 0x26u),
      .textureCoordinates =
          {
              core.mem_r16(address + 0x28u),
              core.mem_r16(address + 0x2Au),
              core.mem_r16(address + 0x2Cu),
              core.mem_r16(address + 0x2Eu),
          },
  };
}

std::optional<SpriteQuadDraw> captureSpriteQuad(Core &core, const SpriteQuadCall &call) {
  const std::uint32_t displayDescriptor = core.mem_r32(kDisplayDescriptor);
  if (displayDescriptor == 0) {
    return std::nullopt;
  }
  return decodeSpriteQuad(readDescriptor(core, call.descriptor),
                          call,
                          static_cast<std::int16_t>(core.mem_r16(displayDescriptor + 4u)),
                          static_cast<std::int32_t>(core.mem_r32(kGlobalFade)));
}

void recordCapturedSpriteQuad(Core &core, const std::optional<SpriteQuadDraw> &draw) {
  if (draw && frameDriver(core).sceneSnapshots().current().valid) {
    frameDriver(core).sceneSnapshots().record(*draw);
  }
}

void spriteQuadCapture(Core *core) {
  const std::uint32_t stackPointer = core->r[29];
  const SpriteQuadCall call{
      .sourceFunction = kSpriteQuadSubmit,
      .descriptor = core->r[4],
      .renderList = core->mem_r32(kSpriteRenderList),
      .packedPosition = core->r[5],
      .orderingBin = static_cast<std::int32_t>(core->r[6]),
      .colors =
          {
              core->r[7],
              core->mem_r32(stackPointer + 0x10u),
              core->mem_r32(stackPointer + 0x14u),
              core->mem_r32(stackPointer + 0x18u),
          },
      .gouraud = true,
  };
  const std::optional<SpriteQuadDraw> draw = captureSpriteQuad(*core, call);

  // The super remains the guest-behavior oracle and owns the function's return value, allocation,
  // packet construction, and OT insertion. Native rendering consumes only the pre-super record above.
  gen_func_8002992C(core);
  recordCapturedSpriteQuad(*core, draw);
}

void flatSpriteQuadCapture(Core *core) {
  const std::uint32_t color = core->r[7];
  const SpriteQuadCall call{
      .sourceFunction = kFlatSpriteQuadSubmit,
      .descriptor = core->r[4],
      .renderList = core->mem_r32(kSpriteRenderList),
      .packedPosition = core->r[5],
      .orderingBin = static_cast<std::int32_t>(core->r[6]),
      .colors = {color, color, color, color},
      .gouraud = false,
  };
  const std::optional<SpriteQuadDraw> draw = captureSpriteQuad(*core, call);

  gen_func_80029D28(core);
  recordCapturedSpriteQuad(*core, draw);
}
#endif

} // namespace

void registerSpriteQuadCaptureOverride() {
#ifdef CRASHBASH_HAVE_SUBSTRATE
  overrides::install(
      kSpriteQuadSubmit, "CrashBash::SpriteQuadCapture", spriteQuadCapture, gen_func_8002992C, shard_set_override);
  overrides::install(kFlatSpriteQuadSubmit,
                     "CrashBash::FlatSpriteQuadCapture",
                     flatSpriteQuadCapture,
                     gen_func_80029D28,
                     shard_set_override);
#else
  lucent::debug("crashbash-render", "sprite-quad capture override deferred: no generated substrate");
#endif
}

} // namespace crashbash::render
