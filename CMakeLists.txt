cmake_minimum_required(VERSION 3.24)
project(crash_bash_port LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(CTest)

# PSXPORT_DIR — the framework. external/psxport is NOT a git submodule (2026-08-16): it is a symlink
# to the workspace's shared framework clone, or a private clone at this repo's psxport.pin on a fresh
# machine; tools/psxport_sync.py --auto establishes whichever applies. Defaults to external/psxport so
# a bare clone of this repo builds standalone.
set(PSXPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/psxport" CACHE PATH
    "psxport framework source directory (default: external/psxport)")
if(NOT EXISTS "${PSXPORT_DIR}/cmake/psxport.cmake")
  message(FATAL_ERROR "PSXPORT_DIR=${PSXPORT_DIR} has no cmake/psxport.cmake — run "
                      "`uv run --frozen python tools/psxport_sync.py --auto`, or point PSXPORT_DIR "
                      "at a psxport checkout.")
endif()
# RECORD WHAT THIS CONFIGURE RESOLVED, so `tools/psxport_sync.py --check` can compare the built
# framework against the one psxport.pin RECORDS.
execute_process(COMMAND git -C "${PSXPORT_DIR}" rev-parse HEAD
                OUTPUT_VARIABLE PSXPORT_RESOLVED_SHA OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET RESULT_VARIABLE _psxport_rev_rc)
if(NOT _psxport_rev_rc EQUAL 0)
  set(PSXPORT_RESOLVED_SHA "unknown")
endif()
file(WRITE "${CMAKE_BINARY_DIR}/psxport_resolved.txt"
     "dir = ${PSXPORT_DIR}\ncommit = ${PSXPORT_RESOLVED_SHA}\n")
set(PSXPORT_BUILD_SMOKE OFF CACHE BOOL "Build the framework agnosticism smoke" FORCE)
add_subdirectory("${PSXPORT_DIR}" psxport_build)

set(CRASHBASH_SEAM_SOURCES
  game/core/boot_object_callbacks.cpp
  game/core/boot_logo_skip.cpp
  game/core/cd_file_read.cpp
  game/core/cd_license_startup.cpp
  game/core/cd_startup.cpp
  game/core/crashbash_boot.cpp
  game/core/crashbash_frame_driver.cpp
  game/core/display_frame.cpp
  game/core/gpu_timeout.cpp
  game/core/memory_card_startup.cpp
  game/core/native_owner_set.cpp
  game/core/polar_push_contact.cpp
  game/diagnostics/menu_boundary.cpp
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
  game/render/interpolated_scene.cpp
  game/render/native_model_producer.cpp
  game/render/native_sprite_quad_producer.cpp
  game/render/scene_snapshot.cpp
  game/render/sprite_quad_capture.cpp
  game/render/sprite_quad_decode.cpp
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

add_library(crashbash_seam OBJECT ${CRASHBASH_SEAM_SOURCES})
set_target_properties(crashbash_seam PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_include_directories(crashbash_seam PRIVATE game/core game/diagnostics game/input game/render)
target_link_libraries(crashbash_seam PRIVATE psxport)
add_custom_target(crashbash_scaffold DEPENDS crashbash_seam psxport)

# Break-first migration: the generated-code product is intentionally gone. Keep one named failure
# until psxport connects Lightrec and exposes loaded-image lifecycle binding for the title adapter.
add_custom_target(
  crashbash_port
  COMMAND
    "${CMAKE_COMMAND}" -E echo
    "Crash Bash gameplay target unavailable: psxport Lightrec backend and loaded-image lifecycle are not integrated"
  COMMAND "${CMAKE_COMMAND}" -E false
  VERBATIM
)
if(BUILD_TESTING)
  add_executable(
    crashbash_touch_controls_test
    tests/test_crashbash_touch_controls.cpp
    game/input/crashbash_touch_controls.cpp
  )
  target_include_directories(crashbash_touch_controls_test PRIVATE game/input)
  set_target_properties(
    crashbash_touch_controls_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_touch_controls COMMAND crashbash_touch_controls_test)
  add_executable(
    crashbash_scene_snapshot_test
    tests/test_scene_snapshot.cpp
    game/render/scene_snapshot.cpp
  )
  target_include_directories(crashbash_scene_snapshot_test PRIVATE game/render)
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
  target_include_directories(crashbash_sprite_quad_decode_test PRIVATE game/render)
  set_target_properties(
    crashbash_sprite_quad_decode_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_sprite_quad_decode COMMAND crashbash_sprite_quad_decode_test)
  add_executable(crashbash_sprite_render_list_test tests/test_sprite_render_list.cpp)
  target_include_directories(crashbash_sprite_render_list_test PRIVATE game/render)
  set_target_properties(
    crashbash_sprite_render_list_test
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME crashbash_sprite_render_list COMMAND crashbash_sprite_render_list_test)
  add_executable(crashbash_render_capabilities_test tests/test_render_capabilities.cpp)
  target_include_directories(crashbash_render_capabilities_test PRIVATE game/core "${PSXPORT_DIR}/tests")
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
  target_include_directories(crashbash_model_face_coverage_test PRIVATE game/render)
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
  target_include_directories(crashbash_model_depth_scale_capture_test PRIVATE game/core game/render)
  target_link_libraries(crashbash_model_depth_scale_capture_test PRIVATE psxport)
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
  target_include_directories(crashbash_model_face_pixel_diagnostic_test PRIVATE game/render)
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
  target_include_directories(crashbash_model_material_diagnostic_test PRIVATE game/render)
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
  target_include_directories(crashbash_model_packet_identity_diagnostic_test PRIVATE game/core game/render)
  target_link_libraries(crashbash_model_packet_identity_diagnostic_test PRIVATE psxport)
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
  target_include_directories(crashbash_model_recipe_capture_test PRIVATE game/render)
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
  target_include_directories(crashbash_model_frame_source_test PRIVATE game/render)
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
  target_include_directories(crashbash_model_transform_math_test PRIVATE game/core game/render)
  target_link_libraries(crashbash_model_transform_math_test PRIVATE psxport)
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
  target_include_directories(crashbash_model_transform_input_diagnostic_test PRIVATE game/core game/render)
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
    game/core/polar_push_contact.cpp
  )
  target_include_directories(crashbash_polar_push_contact_test PRIVATE game/core)
  target_link_libraries(crashbash_polar_push_contact_test PRIVATE psxport)
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
      --allow-empty-scaffold
  )
  add_test(
    NAME crashbash_provision_selftest
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_provision.py"
  )
  add_test(
    NAME crashbash_psxport_pin
    COMMAND
      "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/psxport_sync.py"
      --check --build-dir "${CMAKE_BINARY_DIR}"
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
endif()
