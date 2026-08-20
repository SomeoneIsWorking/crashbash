cmake_minimum_required(VERSION 3.24)
project(crash_bash_port LANGUAGES C CXX)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR "Crash Bash C++ must be compiled with clang++; got ${CMAKE_CXX_COMPILER_ID} "
                      "(${CMAKE_CXX_COMPILER}). Reconfigure with CC=clang CXX=clang++.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
enable_testing()

# PSXPORT_DIR — the framework. external/psxport is NOT a git submodule (2026-08-16): it is a symlink
# to the workspace's shared framework clone, or a private clone at this repo's psxport.pin on a fresh
# machine; tools/psxport_sync.py --auto establishes whichever applies. Defaults to external/psxport so
# a bare clone of this repo builds standalone.
set(PSXPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/psxport" CACHE PATH
    "psxport framework source directory (default: external/psxport)")
if(NOT EXISTS "${PSXPORT_DIR}/cmake/psxport.cmake")
  message(FATAL_ERROR "PSXPORT_DIR=${PSXPORT_DIR} has no cmake/psxport.cmake — run "
                      "`python3 tools/psxport_sync.py --auto`, or point PSXPORT_DIR at a psxport checkout.")
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
set(PSXPORT_BUILD_SMOKE ON CACHE BOOL "Build the framework agnosticism smoke" FORCE)
add_subdirectory("${PSXPORT_DIR}" psxport_build)
add_custom_target(crashbash_scaffold DEPENDS psxport_smoke)
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
