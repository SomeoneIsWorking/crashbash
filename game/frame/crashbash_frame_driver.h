#pragma once

#include "dev_arena.h"
#include "execution_exit.h"
#include "game_runtime.h"
#include "model_face_pixel_diagnostic.h"
#include "model_material_diagnostic.h"
#include "model_packet_identity_diagnostic.h"
#include "model_transform_capture.h"
#include "model_transform_input_diagnostic.h"
#include "polar_push_contact.h"
#include "scene_snapshot.h"

#include <cstdint>

class Core;
class Game;

namespace crashbash {

class CrashBashFrameDriver final : public FrameDriver {
public:
  explicit CrashBashFrameDriver(Game &game);

  void stepFrame(Core &core, std::uint32_t frame) override;

  // Called at the exact point native DisplayFrame removes retail VSync(fields). This owns field
  // callbacks and audio; presentation remains the single commit at the frame-driver tail.
  void deliverDisplayFields(Core &core, std::uint32_t fields);

  render::SceneSnapshotHistory &sceneSnapshots();

  // The developer arena request the control channel arms and this driver applies. Owned here, so the
  // armed request and the frame that advances it are the same object.
  debug::DevArena &devArena() {
    return devArena_;
  }

  // The per-frame render state below is owned here because this driver is the one per-Core object
  // every native override can reach from its bare `Core *`, and because each of these is scoped to a
  // frame this driver brackets. They are product capture (the transform handoff) and evidence (the
  // four diagnostics); none of them is process state.
  // The Polar Push contact traversal's running totals. They live here because this driver is the
  // one per-Core title object a native override can reach from its bare `Core *`, and because a
  // process-global tally would let two Cores report each other's numbers.
  polar::ContactCensus &polarContactCensus() {
    return polarContactCensus_;
  }
  render::ModelTransformCapture &modelTransformCapture() {
    return modelTransformCapture_;
  }
  render::MaterialDiagnostic &materialDiagnostic() {
    return materialDiagnostic_;
  }
  render::FacePixelDiagnostic &facePixelDiagnostic() {
    return facePixelDiagnostic_;
  }
  render::PacketIdentityDiagnostic &packetIdentityDiagnostic() {
    return packetIdentityDiagnostic_;
  }
  const render::TransformInputDiagnostic &transformInputDiagnostic() const {
    return transformInputDiagnostic_;
  }
  render::TransformInputDiagnostic &transformInputDiagnostic() {
    return transformInputDiagnostic_;
  }

private:
  static inline constexpr std::uint32_t kUpdateReturnPc = 0x80027144u;
  static inline constexpr std::uint32_t kPresentReturnPc = 0x80027154u;

  void enterProcessState(Core &core, std::uint32_t state);
  // Requires the completed update and then runs the pair's present. The update itself is carried to
  // its return address by runtime::runGuestCallToReturn, so this only ever sees a completed call.
  void finishUpdateSlice(Core &core);
  // Progress reporting. The interesting event is a process-state CHANGE, and a run that never
  // changes state is precisely the failure worth seeing — so the boring case (sitting in one state)
  // is what gets capped, never the transitions. reportProgress() is called unconditionally at the
  // end of every frame and always prints something for a state it has dwelled in, so "no output"
  // cannot be confused with "no state machine ran".
  void reportProgress(Core &core, std::uint32_t frame);

  Game &game_;
  std::uint32_t activeState_ = 0;
  std::uint32_t deliveredFields_ = 0;
  std::uint32_t stateEntries_ = 0;
  std::uint32_t dwellFrames_ = 0;
  std::uint32_t dwellReports_ = 0;
  std::uint32_t updateFn_ = 0;
  std::uint32_t presentFn_ = 0;
  // The root scene the shell dispatches through (`kAppModeVtable`) and the live scene machine
  // (`0x8009F658`) that actually selects boot / menu / gameplay. Both are read only.
  std::uint32_t appMode_ = 0;
  std::uint32_t appModeChanges_ = 0;
  std::uint32_t scene_ = 0;
  std::uint32_t sceneChanges_ = 0;
  debug::DevArena devArena_;
  polar::ContactCensus polarContactCensus_;
  render::ModelTransformCapture modelTransformCapture_;
  render::MaterialDiagnostic materialDiagnostic_;
  render::FacePixelDiagnostic facePixelDiagnostic_;
  render::PacketIdentityDiagnostic packetIdentityDiagnostic_;
  render::TransformInputDiagnostic transformInputDiagnostic_;
  render::SceneSnapshotHistory sceneSnapshots_;
};

CrashBashFrameDriver &frameDriver(Core &core);

} // namespace crashbash
