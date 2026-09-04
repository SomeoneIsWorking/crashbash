#include "sprite_quad_capture.h"

#include "core.h"
#include "crashbash_frame_driver.h"
#include "game.h"
#include "guest_execution.h"
#include "sprite_quad_decode.h"

#include <cstddef>
#include <cstdint>
#include <lucent/log.h>

namespace crashbash::render {
namespace {

constexpr std::uint32_t kSpriteQuadSubmit = 0x8002992Cu;
constexpr std::uint32_t kFlatSpriteQuadSubmit = 0x80029D28u;
constexpr std::uint32_t kScreenColorQuadSubmit = 0x8001A0D8u;
constexpr std::uint32_t kDisplayDescriptor = 0x8005B698u;
constexpr std::uint32_t kGlobalFade = 0x800569ACu;
constexpr std::uint32_t kScreenXOffset = 0x800569C0u;
constexpr std::uint32_t kScreenYOffset = 0x800569C4u;
constexpr std::uint32_t kSpriteRenderList = 0x800569D8u;
constexpr std::uint32_t kScreenDepthBias = 0x800569DCu;
constexpr std::uint32_t kScreenDepthLimit = 0x800569DEu;

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
  runtime::callOriginal(*core, runtime::GuestImage::Resident, kSpriteQuadSubmit);
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

  runtime::callOriginal(*core, runtime::GuestImage::Resident, kFlatSpriteQuadSubmit);
  recordCapturedSpriteQuad(*core, draw);
}

void screenColorQuadCapture(Core *core) {
  const std::uint32_t source = core->r[4];
  const std::uint32_t flags = core->r[5];
  const std::uint32_t displayDescriptor = core->mem_r32(kDisplayDescriptor);
  std::optional<SpriteQuadDraw> draw;
  if (displayDescriptor != 0 && (flags & 0x00008000u) != 0 && (flags & 0x10000000u) != 0) {
    ScreenColorQuadCall call{
        .sourceFunction = kScreenColorQuadSubmit,
        .sourceAddress = source,
        .renderList = core->mem_r32(kSpriteRenderList),
        .flags = flags,
        .xOffset = static_cast<std::int32_t>(core->mem_r32(kScreenXOffset)),
        .yOffset = static_cast<std::int32_t>(core->mem_r32(kScreenYOffset)),
        .displayScale = static_cast<std::int16_t>(core->mem_r16(displayDescriptor + 4u)),
        .depthBias = static_cast<std::int16_t>(core->mem_r16(kScreenDepthBias)),
        .depthLimit = static_cast<std::int16_t>(core->mem_r16(kScreenDepthLimit)),
        .fade = static_cast<std::int32_t>(core->mem_r32(kGlobalFade)),
    };
    for (std::size_t index = 0; index < call.x.size(); ++index) {
      const std::uint32_t vertex = source + static_cast<std::uint32_t>(index * 8u);
      const std::uint32_t color = source + 0x20u + static_cast<std::uint32_t>(index * 4u);
      call.x[index] = static_cast<std::int16_t>(core->mem_r16(vertex));
      call.y[index] = static_cast<std::int16_t>(core->mem_r16(vertex + 2u));
      call.colors[index] = core->mem_r8(color) | (static_cast<std::uint32_t>(core->mem_r8(color + 1u)) << 8u) |
                           (static_cast<std::uint32_t>(core->mem_r8(color + 2u)) << 16u);
    }
    draw = decodeScreenColorQuad(call);
    if (draw) {
      const int nativeWidth = core->game->gpu.s_disp_w > 0 ? core->game->gpu.s_disp_w : 320;
      const int nativeHeight = core->game->gpu.s_disp_h > 0 ? core->game->gpu.s_disp_h : 240;
      draw->centered4x3Composition =
          isCentered4x3Composition(draw->x[1] - draw->x[0] + 1, draw->y[2] - draw->y[0] + 1, nativeWidth, nativeHeight);
    }
  }

  runtime::callOriginal(*core, runtime::GuestImage::Resident, kScreenColorQuadSubmit);
  recordCapturedSpriteQuad(*core, draw);
}
} // namespace

void registerSpriteQuadCaptureOverride(Core &core) {
  runtime::registerNativeOverride(
      core, runtime::GuestImage::Resident, kSpriteQuadSubmit, "CrashBash::SpriteQuadCapture", spriteQuadCapture);
  runtime::registerNativeOverride(core,
                                  runtime::GuestImage::Resident,
                                  kFlatSpriteQuadSubmit,
                                  "CrashBash::FlatSpriteQuadCapture",
                                  flatSpriteQuadCapture);
  runtime::registerNativeOverride(core,
                                  runtime::GuestImage::Resident,
                                  kScreenColorQuadSubmit,
                                  "CrashBash::ScreenColorQuadCapture",
                                  screenColorQuadCapture);
}

} // namespace crashbash::render
