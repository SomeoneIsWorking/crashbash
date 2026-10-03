cmake_minimum_required(VERSION 3.24)
project(crash_bash_port LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(CTest)

# PSXPORT_DIR — the framework. external/psxport is NOT a git submodule (2026-08-16): it is a relative
# symlink to the workspace's live framework checkout, or a clone of its main where there is none;
# tools/psxport_fetch.py --auto establishes whichever applies. Defaults to external/psxport so
# a bare clone of this repo builds standalone.
set(PSXPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/psxport" CACHE PATH
    "psxport framework source directory (default: external/psxport)")
if(NOT EXISTS "${PSXPORT_DIR}/cmake/psxport.cmake")
  message(FATAL_ERROR "PSXPORT_DIR=${PSXPORT_DIR} has no cmake/psxport.cmake — run "
                      "`uv run --frozen python tools/psxport_fetch.py --auto`, or point PSXPORT_DIR "
                      "at a psxport checkout.")
endif()
set(PSXPORT_BUILD_SMOKE OFF CACHE BOOL "Build the framework agnosticism smoke" FORCE)
add_subdirectory("${PSXPORT_DIR}" psxport_build)

# These are non-executable identity facts from the same manifest used by provisioning.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS titles/crashbash/executable.json)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/crashbash/executable.json" CRASHBASH_EXECUTABLE_MANIFEST)
string(JSON CRASHBASH_EXECUTABLE_SHA256 GET "${CRASHBASH_EXECUTABLE_MANIFEST}" sha256)
string(JSON CRASHBASH_EXECUTABLE_SIZE GET "${CRASHBASH_EXECUTABLE_MANIFEST}" file_size)
string(JSON CRASHBASH_EXECUTABLE_ENTRY GET "${CRASHBASH_EXECUTABLE_MANIFEST}" header entry)
string(JSON CRASHBASH_EXECUTABLE_TEXT GET "${CRASHBASH_EXECUTABLE_MANIFEST}" header text_address)
string(JSON CRASHBASH_EXECUTABLE_TEXT_SIZE GET "${CRASHBASH_EXECUTABLE_MANIFEST}" header text_size)
configure_file(game/execution/executable_identity.h.in title_metadata/executable_identity.h @ONLY)

function(configure_crashbash_module_identity module_name)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "titles/crashbash/${module_name}_module.json")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/crashbash/${module_name}_module.json" MODULE_MANIFEST)
  string(JSON MODULE_DISC_LBA GET "${MODULE_MANIFEST}" payload_disc_lba)
  string(JSON MODULE_SECTOR_COUNT GET "${MODULE_MANIFEST}" sector_count)
  string(JSON MODULE_PAYLOAD_BYTES GET "${MODULE_MANIFEST}" payload_size)
  string(JSON MODULE_SHA256 GET "${MODULE_MANIFEST}" payload_sha256)
  string(JSON MODULE_LOAD_ADDRESS GET "${MODULE_MANIFEST}" load_address)
  # The nested gameplay overlays record `entry_pointer_offset` and `entry` as JSON null, because their
  # payload headers are ASCII name tables rather than a table of contents (measured 2026-09-27; see
  # authenticated_module_image.h). `string(JSON ... GET)` is an ERROR on a null member, so the type is
  # asked first and the absence is carried through as a stated zero rather than a configure failure or
  # a guessed offset. The two are recorded as hex STRINGS wherever they exist, so the test is against
  # NULL and not against a numeric type: testing for NUMBER silently zeroes BOOT's and MENU's real
  # entry witnesses, which is exactly the regression this branch was written to avoid.
  string(JSON MODULE_ENTRY_OFFSET_TYPE TYPE "${MODULE_MANIFEST}" entry_pointer_offset)
  string(JSON MODULE_ENTRY_TYPE TYPE "${MODULE_MANIFEST}" entry)
  if(NOT MODULE_ENTRY_OFFSET_TYPE STREQUAL "NULL" AND NOT MODULE_ENTRY_TYPE STREQUAL "NULL")
    string(JSON MODULE_ENTRY_OFFSET GET "${MODULE_MANIFEST}" entry_pointer_offset)
    string(JSON MODULE_ENTRY GET "${MODULE_MANIFEST}" entry)
  else()
    set(MODULE_ENTRY_OFFSET 0)
    set(MODULE_ENTRY 0)
  endif()
  set(MODULE_NAME "${module_name}")
  configure_file(game/execution/module_image_identity.h.in "title_metadata/${module_name}_image_identity.h" @ONLY)
endfunction()

configure_crashbash_module_identity(boot)
configure_crashbash_module_identity(menu)
configure_crashbash_module_identity(dat28136)
configure_crashbash_module_identity(dat28272)
configure_crashbash_module_identity(dat28241)
configure_crashbash_module_identity(dat28382)
configure_crashbash_module_identity(dat22510)

# Every first-party header root. Each game/<subsystem> directory owns one concept; this list is what
# lets a source name a header without naming the directory it lives in.
set(CRASHBASH_TITLE_INCLUDE_DIRS
  game/boot
  game/debug
  game/diagnostics
  game/disc
  game/entry
  game/execution
  game/frame
  game/gameplay
  game/input
  game/render
  game/render/fps60
  game/render/widescreen
  game/title
)

set(CRASHBASH_SEAM_SOURCES
  game/boot/boot_object_callbacks.cpp
  game/boot/boot_logo_skip.cpp
  game/boot/authenticated_module_image.cpp
  game/boot/nested_module_image.cpp
  game/boot/boot_image.cpp
  game/disc/cd_file_read.cpp
  game/disc/loading_card_skip.cpp
  game/disc/cd_license_startup.cpp
  game/disc/cd_startup.cpp
  game/boot/crashbash_boot.cpp
  game/frame/crashbash_frame_driver.cpp
  game/input/crashbash_input_phase.cpp
  game/debug/dev_arena.cpp
  game/frame/display_frame.cpp
  game/frame/gpu_timeout.cpp
  game/boot/memory_card_startup.cpp
  game/boot/menu_image.cpp
  game/execution/native_owner_set.cpp
  game/execution/title_adapter.cpp
  game/boot/resident_image.cpp
  game/gameplay/polar_push_contact.cpp
  game/diagnostics/menu_boundary.cpp
  game/diagnostics/scene_machine.cpp
  game/input/crashbash_touch_controls.cpp
  game/render/model_face_coverage.cpp
  game/render/model_face_pixel_diagnostic.cpp
  game/render/model_depth_scale_capture.cpp
  game/render/model_frame_source.cpp
  game/render/model_material_diagnostic.cpp
  game/render/model_packet_identity_diagnostic.cpp
  game/render/model_submit_capture.cpp
  game/render/model_recipe_capture.cpp
  game/render/model_transform_capture.cpp
  game/render/model_transform_input_diagnostic.cpp
  game/render/fps60/interpolated_scene.cpp
  game/render/native_model_producer.cpp
  game/render/native_sprite_quad_producer.cpp
  game/render/render_viewport.cpp
  game/render/fps60/scene_snapshot.cpp
  game/render/sprite_quad_capture.cpp
  game/render/sprite_quad_decode.cpp
  game/render/widescreen/ui_anchor.cpp
)

find_package(SDL3 REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(
  crashbash_appimage_launcher
  packaging/linux/launcher_main.cpp
  packaging/linux/install_media.cpp
  packaging/linux/user_paths.cpp
  "${PSXPORT_DIR}/vendor/lucent/src/zip.cpp"
)
target_include_directories(
  crashbash_appimage_launcher
  PRIVATE
    packaging/linux
    "${PSXPORT_DIR}/vendor/lucent/include"
)
target_link_libraries(crashbash_appimage_launcher PRIVATE lucent SDL3::SDL3 ZLIB::ZLIB)
set_target_properties(
  crashbash_appimage_launcher
  PROPERTIES
    OUTPUT_NAME crashbash-launcher
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

add_library(crashbash_guest_execution STATIC game/execution/guest_execution.cpp
                                            game/execution/image_identity_state.cpp)
set_target_properties(crashbash_guest_execution PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_include_directories(crashbash_guest_execution PUBLIC game/execution game/boot game/title game/disc)
target_link_libraries(crashbash_guest_execution PUBLIC psxport)

# The whole-run ledger is its own owner, and its own library, because BOTH the execution seam
# (`guest_execution.cpp` feeds it original calls, faults and refusals) and the image-publication
# owner (which feeds it every authenticated generation) depend on it. Folding it into either of them
# would make one of the two a cycle.
add_library(crashbash_run_ledger STATIC game/diagnostics/run_ledger.cpp)
set_target_properties(crashbash_run_ledger PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_include_directories(crashbash_run_ledger PUBLIC game/diagnostics)
target_link_libraries(crashbash_run_ledger PUBLIC psxport)
target_link_libraries(crashbash_guest_execution PUBLIC crashbash_run_ledger)

add_library(crashbash_seam OBJECT ${CRASHBASH_SEAM_SOURCES})
set_target_properties(crashbash_seam PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_include_directories(crashbash_seam PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS} "${CMAKE_CURRENT_BINARY_DIR}/title_metadata")
target_link_libraries(crashbash_seam PUBLIC crashbash_guest_execution)
add_custom_target(crashbash_scaffold DEPENDS crashbash_seam psxport)

# The native/dynarec player entry is deliberately thin: title authentication and per-Core setup
# stay in the title owners while psxport owns the bounded host frame loop.
add_executable(crashbash_port game/entry/player_entry.cpp $<TARGET_OBJECTS:crashbash_seam>)
target_include_directories(crashbash_port PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS}
                           "${CMAKE_CURRENT_BINARY_DIR}/title_metadata")
target_link_libraries(crashbash_port PRIVATE crashbash_guest_execution psxport)
set_target_properties(
  crashbash_port
  PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(BUILD_TESTING)
  add_executable(crashbash_title_adapter_test tests/test_title_adapter.cpp)
  target_include_directories(crashbash_title_adapter_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS} "${PSXPORT_DIR}/tests" "${CMAKE_CURRENT_BINARY_DIR}/title_metadata")
  target_link_libraries(crashbash_title_adapter_test PRIVATE crashbash_seam)
  set_target_properties(crashbash_title_adapter_test PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
  add_test(NAME crashbash_title_adapter COMMAND crashbash_title_adapter_test)
  set_tests_properties(crashbash_title_adapter PROPERTIES
    TIMEOUT 30
    ENVIRONMENT "PSXPORT_CARD=${CMAKE_CURRENT_SOURCE_DIR}/scratch/title-adapter-test/card.mcr")
  add_executable(crashbash_input_phase_test tests/test_input_phase.cpp)
  target_include_directories(crashbash_input_phase_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS} "${PSXPORT_DIR}/tests")
  target_link_libraries(crashbash_input_phase_test PRIVATE crashbash_seam)
  set_target_properties(crashbash_input_phase_test PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
  add_test(NAME crashbash_input_phase COMMAND crashbash_input_phase_test)
  add_executable(crashbash_run_ledger_test tests/test_run_ledger.cpp)
  target_include_directories(crashbash_run_ledger_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS}
                             "${PSXPORT_DIR}/tests"
                             "${CMAKE_CURRENT_BINARY_DIR}/title_metadata")
  target_link_libraries(crashbash_run_ledger_test PRIVATE crashbash_seam)
  set_target_properties(crashbash_run_ledger_test PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
  add_test(NAME crashbash_run_ledger COMMAND crashbash_run_ledger_test)
  add_executable(crashbash_boot_image_test tests/test_boot_image.cpp)
  target_include_directories(crashbash_boot_image_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS}
                           "${PSXPORT_DIR}/tests"
                           "${CMAKE_CURRENT_BINARY_DIR}/title_metadata")
  target_link_libraries(crashbash_boot_image_test PRIVATE crashbash_seam)
  set_target_properties(crashbash_boot_image_test PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
  add_test(NAME crashbash_boot_image COMMAND crashbash_boot_image_test)
  add_executable(crashbash_menu_image_test tests/test_menu_image.cpp)
  target_include_directories(crashbash_menu_image_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS}
                           "${PSXPORT_DIR}/tests"
                           "${CMAKE_CURRENT_BINARY_DIR}/title_metadata")
  target_link_libraries(crashbash_menu_image_test PRIVATE crashbash_seam)
  set_target_properties(crashbash_menu_image_test PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
  add_test(NAME crashbash_menu_image COMMAND crashbash_menu_image_test)
  add_executable(crashbash_nested_module_image_test tests/test_nested_module_image.cpp)
  target_include_directories(crashbash_nested_module_image_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS}
                                                                    "${PSXPORT_DIR}/tests"
                                                                    "${CMAKE_CURRENT_BINARY_DIR}/title_metadata")
  target_link_libraries(crashbash_nested_module_image_test PRIVATE crashbash_seam)
  set_target_properties(crashbash_nested_module_image_test PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
  add_test(NAME crashbash_nested_module_image COMMAND crashbash_nested_module_image_test)
  add_executable(crashbash_guest_execution_test tests/test_guest_execution.cpp)
  target_include_directories(crashbash_guest_execution_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS} "${PSXPORT_DIR}/tests")
  target_link_libraries(crashbash_guest_execution_test PRIVATE crashbash_guest_execution)
  set_target_properties(crashbash_guest_execution_test PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
  add_test(NAME crashbash_guest_execution COMMAND crashbash_guest_execution_test)
  # The scene machine is read out of the AUTHENTICATED images, so its cases need the same
  # provisioned inputs the other image tests take on argv. There is deliberately NO "skip when
  # absent" branch: a gate that registers nothing when its corpus is missing is the green zero this
  # workspace has been bitten by, and a test that cannot run must read as a failure, not a pass.
  add_executable(crashbash_scene_machine_test tests/test_scene_machine.cpp)
  target_include_directories(crashbash_scene_machine_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS}
                                                                  "${PSXPORT_DIR}/tests"
                                                                  "${CMAKE_CURRENT_BINARY_DIR}/title_metadata")
  target_link_libraries(crashbash_scene_machine_test PRIVATE crashbash_seam)
  set_target_properties(crashbash_scene_machine_test PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
  add_test(NAME crashbash_scene_machine
           COMMAND crashbash_scene_machine_test
                   "${CMAKE_CURRENT_SOURCE_DIR}/scratch/bin/crashbash/SCUS_945.70"
                   "${CMAKE_CURRENT_SOURCE_DIR}/scratch/bin/crashbash/overlays/BOOT.BIN")
  add_executable(
    crashbash_touch_controls_test
    tests/test_crashbash_touch_controls.cpp
    game/input/crashbash_touch_controls.cpp
  )
  target_include_directories(crashbash_touch_controls_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  set_target_properties(
    crashbash_touch_controls_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_touch_controls COMMAND crashbash_touch_controls_test)
  add_executable(
    crashbash_scene_snapshot_test
    tests/test_scene_snapshot.cpp
    game/render/fps60/scene_snapshot.cpp
  )
  target_include_directories(crashbash_scene_snapshot_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  target_link_libraries(crashbash_scene_snapshot_test PRIVATE psxport)
  set_target_properties(
    crashbash_scene_snapshot_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_scene_snapshot COMMAND crashbash_scene_snapshot_test)
  add_executable(
    crashbash_sprite_quad_decode_test
    tests/test_sprite_quad_decode.cpp
    game/render/sprite_quad_decode.cpp
  )
  target_include_directories(crashbash_sprite_quad_decode_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  set_target_properties(
    crashbash_sprite_quad_decode_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_sprite_quad_decode COMMAND crashbash_sprite_quad_decode_test)
  add_executable(crashbash_sprite_render_list_test tests/test_sprite_render_list.cpp)
  target_include_directories(crashbash_sprite_render_list_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  set_target_properties(
    crashbash_sprite_render_list_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_sprite_render_list COMMAND crashbash_sprite_render_list_test)
  add_executable(crashbash_render_viewport_test tests/test_render_viewport.cpp game/render/render_viewport.cpp)
  target_include_directories(crashbash_render_viewport_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  set_target_properties(
    crashbash_render_viewport_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_render_viewport COMMAND crashbash_render_viewport_test)
  add_executable(crashbash_ui_anchor_test tests/test_ui_anchor.cpp)
  target_include_directories(crashbash_ui_anchor_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  set_target_properties(
    crashbash_ui_anchor_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_ui_anchor COMMAND crashbash_ui_anchor_test)
  add_executable(crashbash_render_capabilities_test tests/test_render_capabilities.cpp)
  target_include_directories(crashbash_render_capabilities_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS} "${PSXPORT_DIR}/tests")
  target_link_libraries(crashbash_render_capabilities_test PRIVATE psxport)
  set_target_properties(
    crashbash_render_capabilities_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_render_capabilities COMMAND crashbash_render_capabilities_test)
  add_executable(
    crashbash_model_face_coverage_test
    tests/test_model_face_coverage.cpp
    game/render/model_face_coverage.cpp
  )
  target_include_directories(crashbash_model_face_coverage_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  target_link_libraries(crashbash_model_face_coverage_test PRIVATE psxport)
  set_target_properties(
    crashbash_model_face_coverage_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_model_face_coverage COMMAND crashbash_model_face_coverage_test)
  add_executable(
    crashbash_model_depth_scale_capture_test
    tests/test_model_depth_scale_capture.cpp
    game/render/model_depth_scale_capture.cpp
  )
  target_include_directories(crashbash_model_depth_scale_capture_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  target_link_libraries(crashbash_model_depth_scale_capture_test PRIVATE crashbash_guest_execution)
  set_target_properties(
    crashbash_model_depth_scale_capture_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_model_depth_scale_capture COMMAND crashbash_model_depth_scale_capture_test)
  add_executable(
    crashbash_model_face_pixel_diagnostic_test
    tests/test_model_face_pixel_diagnostic.cpp
    game/render/model_face_pixel_diagnostic.cpp
  )
  target_include_directories(crashbash_model_face_pixel_diagnostic_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  target_link_libraries(crashbash_model_face_pixel_diagnostic_test PRIVATE psxport)
  set_target_properties(
    crashbash_model_face_pixel_diagnostic_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_model_face_pixel_diagnostic COMMAND crashbash_model_face_pixel_diagnostic_test)
  add_executable(
    crashbash_model_material_diagnostic_test
    tests/test_model_material_diagnostic.cpp
    game/render/model_material_diagnostic.cpp
  )
  target_include_directories(crashbash_model_material_diagnostic_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  target_link_libraries(crashbash_model_material_diagnostic_test PRIVATE psxport)
  set_target_properties(
    crashbash_model_material_diagnostic_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_model_material_diagnostic COMMAND crashbash_model_material_diagnostic_test)
  add_executable(
    crashbash_model_packet_identity_diagnostic_test
    tests/test_model_packet_identity_diagnostic.cpp
    game/render/model_face_coverage.cpp
    game/render/model_packet_identity_diagnostic.cpp
  )
  target_include_directories(crashbash_model_packet_identity_diagnostic_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  target_link_libraries(crashbash_model_packet_identity_diagnostic_test PRIVATE crashbash_guest_execution)
  set_target_properties(
    crashbash_model_packet_identity_diagnostic_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(
    NAME crashbash_model_packet_identity_diagnostic
    COMMAND crashbash_model_packet_identity_diagnostic_test
  )
  add_executable(
    crashbash_model_recipe_capture_test
    tests/test_model_recipe_capture.cpp
    game/render/model_frame_source.cpp
    game/render/model_material_diagnostic.cpp
    game/render/model_recipe_capture.cpp
  )
  target_include_directories(crashbash_model_recipe_capture_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  target_link_libraries(crashbash_model_recipe_capture_test PRIVATE psxport)
  set_target_properties(
    crashbash_model_recipe_capture_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_model_recipe_capture COMMAND crashbash_model_recipe_capture_test)
  add_executable(
    crashbash_model_frame_source_test
    tests/test_model_frame_source.cpp
    game/render/model_frame_source.cpp
  )
  target_include_directories(crashbash_model_frame_source_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  set_target_properties(
    crashbash_model_frame_source_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_model_frame_source COMMAND crashbash_model_frame_source_test)
  add_executable(
    crashbash_model_transform_math_test
    tests/test_model_transform_math.cpp
    game/render/model_transform_capture.cpp
  )
  target_include_directories(crashbash_model_transform_math_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  target_link_libraries(crashbash_model_transform_math_test PRIVATE crashbash_guest_execution)
  set_target_properties(
    crashbash_model_transform_math_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_model_transform_math COMMAND crashbash_model_transform_math_test)
  add_executable(
    crashbash_model_transform_input_diagnostic_test
    tests/test_model_transform_input_diagnostic.cpp
    game/render/model_transform_input_diagnostic.cpp
  )
  target_include_directories(crashbash_model_transform_input_diagnostic_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  target_link_libraries(crashbash_model_transform_input_diagnostic_test PRIVATE psxport)
  set_target_properties(
    crashbash_model_transform_input_diagnostic_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(
    NAME crashbash_model_transform_input_diagnostic
    COMMAND crashbash_model_transform_input_diagnostic_test
  )
  add_executable(
    crashbash_polar_push_contact_test
    tests/test_polar_push_contact.cpp
    game/gameplay/polar_push_contact.cpp
  )
  target_include_directories(crashbash_polar_push_contact_test PRIVATE ${CRASHBASH_TITLE_INCLUDE_DIRS})
  target_link_libraries(crashbash_polar_push_contact_test PRIVATE crashbash_guest_execution)
  set_target_properties(
    crashbash_polar_push_contact_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_polar_push_contact COMMAND crashbash_polar_push_contact_test)
  add_test(
    NAME crashbash_native_ownership_selftest
    COMMAND
      "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_source_policy.py"
  )
  add_test(
    NAME crashbash_native_ownership
    COMMAND
      "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_native_ownership.py"
  )
  add_test(
    NAME crashbash_cpp_policy
    COMMAND
      "${Python3_EXECUTABLE}" "${PSXPORT_DIR}/tools/check_cpp_style.py"
      --root "${CMAKE_CURRENT_SOURCE_DIR}" --compile-commands "${CMAKE_BINARY_DIR}"
  )
  add_test(
    NAME crashbash_bootstrap_selftest
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_bootstrap.py"
  )
  add_test(
    NAME crashbash_provision_selftest
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_provision.py"
  )
  add_test(
    NAME crashbash_oracle_irq_compare_selftest
    COMMAND
      "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_oracle_irq.py"
      --selftest
  )
  add_test(
    NAME crashbash_read_completion_compare_selftest
    COMMAND
      "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_read_completion.py"
      --selftest
  )
  add_test(
    NAME crashbash_command_response_timing_selftest
    COMMAND
      "${Python3_EXECUTABLE}" -B
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_command_response_timing.py" --selftest
  )
  add_test(
    NAME crashbash_bounded_resume_selftest
    COMMAND
      "${Python3_EXECUTABLE}" -B
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_bounded_resume.py" --selftest
  )
endif()

# The caller census's OWN decoder check. `probe_call_sites.py` computed a jal target as
# (word & 0x03FFFFFF) << 2 with no region term, which caps every target at 0x0FFFFFFC -- so `--callee` on
# any real guest address could NEVER match and the tool printed `matched 0` over a full scan, with a
# denominator, readable as a finding. Fixed, and this is what keeps it fixed: the decode needs no disc,
# no product and no root, so it is checkable on every gate run. `--callee 0x800320EC` now matches 47,
# which is exactly the count docs/findings/vsync-owner-map.md records.
add_test(NAME probe_call_sites_selftest
         COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/probe_call_sites.py"
                 --selftest)
