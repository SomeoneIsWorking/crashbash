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

void spriteQuadCapture(Core *core) {
  const std::uint32_t stackPointer = core->r[29];
  const std::uint32_t displayDescriptor = core->mem_r32(kDisplayDescriptor);
  std::optional<SpriteQuadDraw> draw;
  if (displayDescriptor != 0) {
    const SpriteQuadCall call{
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
    };
    draw = decodeSpriteQuad(readDescriptor(*core, call.descriptor),
                            call,
                            static_cast<std::int16_t>(core->mem_r16(displayDescriptor + 4u)),
                            static_cast<std::int32_t>(core->mem_r32(kGlobalFade)));
  }

  // The super remains the guest-behavior oracle and owns the function's return value, allocation,
  // packet construction, and OT insertion. Native rendering consumes only the pre-super record above.
  gen_func_8002992C(core);
  if (draw && frameDriver(*core).sceneSnapshots().current().valid) {
    frameDriver(*core).sceneSnapshots().record(*draw);
  }
}
#endif

} // namespace

void registerSpriteQuadCaptureOverride() {
#ifdef CRASHBASH_HAVE_SUBSTRATE
  overrides::install(
      kSpriteQuadSubmit, "CrashBash::SpriteQuadCapture", spriteQuadCapture, gen_func_8002992C, shard_set_override);
#else
  lucent::debug("crashbash-render", "sprite-quad capture override deferred: no generated substrate");
#endif
}

} // namespace crashbash::render
