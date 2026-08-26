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
  game/core/crashbash_runtime.cpp
  game/core/game_config.cpp
  game/core/game_hooks.cpp
  game/core/main.cpp
  game/core/recomp_register.cpp
  game/diagnostics/menu_boundary.cpp
)
add_library(crashbash_seam OBJECT ${CRASHBASH_SEAM_SOURCES})
set_target_properties(crashbash_seam PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_include_directories(crashbash_seam PRIVATE game/core game/diagnostics)
target_link_libraries(crashbash_seam PRIVATE psxport)
add_custom_target(crashbash_scaffold DEPENDS crashbash_seam psxport)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/generated/rec_sources.cmake")
  include("${CMAKE_CURRENT_SOURCE_DIR}/generated/rec_sources.cmake")
  list(TRANSFORM GEN_REC_SRCS PREPEND generated/)
  set_source_files_properties(
    ${GEN_REC_SRCS}
    PROPERTIES
      LANGUAGE CXX
      COMPILE_OPTIONS "-w;-O1;-foptimize-sibling-calls;-fno-strict-aliasing;-fwrapv"
  )
  add_executable(crashbash_port ${CRASHBASH_SEAM_SOURCES} ${GEN_REC_SRCS})
  target_compile_definitions(crashbash_port PRIVATE CRASHBASH_HAVE_SUBSTRATE=1)
  target_include_directories(crashbash_port PRIVATE game/core game/diagnostics generated)
  target_compile_options(crashbash_port PRIVATE -O2 -g)
  target_link_libraries(crashbash_port PRIVATE psxport)
  add_dependencies(crashbash_port gen_gpu_shaders)
  set_target_properties(
    crashbash_port
    PROPERTIES
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED ON
      ENABLE_EXPORTS ON
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scratch/bin"
  )
else()
  message(STATUS "crashbash_port: generated substrate absent; run tools/recomp_bootstrap.py --ensure")
endif()

if(BUILD_TESTING)
  add_test(
    NAME crashbash_boot_boundary_selftest
    COMMAND
      "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_boot.py" --selftest
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
    NAME crashbash_recomp_bootstrap_selftest
    COMMAND
      "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/recomp_bootstrap.py"
      --selftest --crt0-extract $<TARGET_FILE:crt0_extract>
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
    NAME crashbash_cdc_phase_progress_selftest
    COMMAND
      "${Python3_EXECUTABLE}" -B
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_cdc_phase_progress.py" --selftest
  )
  add_test(
    NAME crashbash_render_anchor_inventory_selftest
    COMMAND
      "${Python3_EXECUTABLE}" -B
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/inventory_render_anchors.py" --selftest
  )
  add_test(
    NAME crashbash_render_anchor_reach_selftest
    COMMAND
      "${Python3_EXECUTABLE}" -B
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_render_anchor_reach.py" --selftest
  )
endif()
