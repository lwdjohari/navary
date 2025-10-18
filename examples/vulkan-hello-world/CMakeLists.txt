cmake_minimum_required(VERSION 3.20)
project(vulkan-hello-world LANGUAGES CXX)

# ------------------------------------------------------------
# Options
# ------------------------------------------------------------
option(ENABLE_ASAN "Enable AddressSanitizer for Debug builds" ON)

# Respect user-provided -DVK_EMBED_LSAN_SUPPRESS=0/1 (we don't auto-enable it)
# Per-binary ASan defaults (used below). Default disables LSAN leak noise.
set(ASAN_DEFAULT_OPTIONS "detect_leaks=0" CACHE STRING
    "String returned by __asan_default_options() for this binary")
option(ASAN_FORCE_OPTS "Force-embed __asan_default_options() TU even if ASan not detected" OFF)

# ------------------------------------------------------------
# Language / generator (no timestamps; deterministic)
# ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer Ninja if available (deterministic: we don't print which path was chosen beyond generator)
if(NOT CMAKE_GENERATOR)
  find_program(NINJA_EXE NAMES ninja ninja-build)
  if(NINJA_EXE)
    set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)
  endif()
endif()

# ------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------
find_package(Vulkan REQUIRED)

# ------------------------------------------------------------
# Target
# ------------------------------------------------------------
add_executable(vulkan-hello-world src/main.cc)
target_link_libraries(vulkan-hello-world PRIVATE Vulkan::Vulkan)

# Warnings
if (MSVC)
  target_compile_options(vulkan-hello-world PRIVATE /W4 /permissive-)
else()
  target_compile_options(vulkan-hello-world PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ------------------------------------------------------------
# AddressSanitizer setup (only for Debug builds, GCC/Clang)
# ------------------------------------------------------------
set(_HAS_ASAN FALSE)
if (ENABLE_ASAN AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(vulkan-hello-world PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(vulkan-hello-world PRIVATE -fsanitize=address)
  set(_HAS_ASAN TRUE)
endif()

# Respect user-provided VK_EMBED_LSAN_SUPPRESS (no auto-on)
if (DEFINED VK_EMBED_LSAN_SUPPRESS)
  target_compile_definitions(vulkan-hello-world PRIVATE VK_EMBED_LSAN_SUPPRESS=${VK_EMBED_LSAN_SUPPRESS})
endif()

# ------------------------------------------------------------
# Per-binary __asan_default_options() TU
#   Default: detect_leaks=0 to silence bogus LSAN Vulkan exit-time leak.
#   Compiled WITHOUT ASan to avoid early-init reentrancy.
# ------------------------------------------------------------
if (_HAS_ASAN OR ASAN_FORCE_OPTS)
  set(_asan_dir "${CMAKE_CURRENT_BINARY_DIR}/_asan_opts")
  file(MAKE_DIRECTORY "${_asan_dir}")
  set(_asan_src "${_asan_dir}/asan_default_options.cc")

  file(WRITE "${_asan_src}"
"extern \"C\" const char* __asan_default_options() noexcept {
  return R\"ASAN(${ASAN_DEFAULT_OPTIONS})ASAN\";
}
")

  target_sources(vulkan-hello-world PRIVATE "${_asan_src}")

  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # Compile ONLY this file without ASan
    set_source_files_properties("${_asan_src}" PROPERTIES
      COMPILE_OPTIONS "-fno-sanitize=address")
  endif()
endif()

# ------------------------------------------------------------
# Output dir
# ------------------------------------------------------------
set_target_properties(vulkan-hello-world PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ------------------------------------------------------------
# Deterministic project summary (no timestamps, fixed ordering)
# ------------------------------------------------------------
# Resolve build type deterministically for single- vs multi-config generators.
set(_SUMMARY_BUILD_TYPE "")
if(DEFINED CMAKE_CONFIGURATION_TYPES)
  # Multi-config generator (e.g., MSVC, Ninja Multi-Config)
  set(_SUMMARY_BUILD_TYPE "Multi-Config")
else()
  if(CMAKE_BUILD_TYPE)
    set(_SUMMARY_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
  else()
    set(_SUMMARY_BUILD_TYPE "Unset")
  endif()
endif()

# Compiler info (always printed in a fixed order)
set(_COMP_ID        "${CMAKE_CXX_COMPILER_ID}")
set(_COMP_VER       "${CMAKE_CXX_COMPILER_VERSION}")
set(_COMP_PATH      "${CMAKE_CXX_COMPILER}")

# Generator and CMake versions
set(_GEN            "${CMAKE_GENERATOR}")
set(_CMAKE_VER      "${CMAKE_VERSION}")

# Vulkan info (best-effort, stable fields only)
set(_VULKAN_FOUND   "")
if (Vulkan_FOUND)
  set(_VULKAN_FOUND "FOUND")
else()
  set(_VULKAN_FOUND "NOT FOUND")
endif()

# If the imported target exposes include dirs, record one canonical path (deterministic)
set(_VULKAN_INC "")
if (TARGET Vulkan::Vulkan)
  get_target_property(_VK_INCLUDES Vulkan::Vulkan INTERFACE_INCLUDE_DIRECTORIES)
  if(_VK_INCLUDES)
    list(GET _VK_INCLUDES 0 _VULKAN_INC)
  endif()
endif()

# ASan/LSan flags (deterministic booleans/strings)
set(_ASAN_ENABLED "${_HAS_ASAN}")
if (DEFINED VK_EMBED_LSAN_SUPPRESS)
  set(_VK_LSAN_DEFINE "${VK_EMBED_LSAN_SUPPRESS}")
else()
  set(_VK_LSAN_DEFINE "not-defined")
endif()

# Print (strict order, no decoration)
message(STATUS "")
message(STATUS "----------------- Project Summary -----------------")
message(STATUS "Project.Name:            ${PROJECT_NAME}")
message(STATUS "Project.Language:        CXX")
message(STATUS "CMake.Version:           ${_CMAKE_VER}")
message(STATUS "Generator:               ${_GEN}")
message(STATUS "CXX.Standard:            ${CMAKE_CXX_STANDARD}")
message(STATUS "Build.Type:              ${_SUMMARY_BUILD_TYPE}")
message(STATUS "Compiler.ID:             ${_COMP_ID}")
message(STATUS "Compiler.Version:        ${_COMP_VER}")
message(STATUS "Compiler.Path:           ${_COMP_PATH}")
message(STATUS "Vulkan.Status:           ${_VULKAN_FOUND}")
message(STATUS "Vulkan.Include:          ${_VULKAN_INC}")
message(STATUS "ASan.Enabled:            ${_ASAN_ENABLED}")
message(STATUS "ASan.DefaultOptions:     ${ASAN_DEFAULT_OPTIONS}")
message(STATUS "VK_EMBED_LSAN_SUPPRESS:  ${_VK_LSAN_DEFINE}")
message(STATUS "Binary.OutputDir:        ${CMAKE_BINARY_DIR}/bin")
message(STATUS "")
