cmake_minimum_required(VERSION 3.20)
project(navary_arena_runner LANGUAGES CXX)


# ==========================================================
# Global binary output directory
# ==========================================================
# This ensures all examples end up in build/bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ==========================================================
# Sanitizer options (opt-in)
# ==========================================================
option(NVR_ASAN  "Enable AddressSanitizer"           OFF)
option(NVR_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(NVR_TSAN  "Enable ThreadSanitizer"            OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# ==========================================================
# Sanitizer flags
# ==========================================================
set(SAN_FLAGS "")
if (NVR_ASAN)
  list(APPEND SAN_FLAGS "-fsanitize=address" "-fno-omit-frame-pointer" "-O1" "-g")
endif()
if (NVR_UBSAN)
  list(APPEND SAN_FLAGS "-fsanitize=undefined" "-fno-omit-frame-pointer" "-O1" "-g")
endif()
if (NVR_TSAN)
  list(APPEND SAN_FLAGS "-fsanitize=thread" "-fno-omit-frame-pointer" "-O1" "-g")
endif()

# ==========================================================
# Helper to create example binaries
# ==========================================================
function(navary_add_example name)
  add_executable(${name} ${ARGN})
  target_compile_options(${name} PRIVATE ${SAN_FLAGS})
  target_link_options(${name} PRIVATE ${SAN_FLAGS})
  target_link_libraries(${name}
    PRIVATE
      navary::engine        # main engine target
      enkiTS                # thirdparty scheduler
      pthread
  )
  target_include_directories(${name} PRIVATE
    ${CMAKE_SOURCE_DIR}
  )
endfunction()


# ==========================================================
# Example binaries
# ==========================================================
navary_add_example(navary_arena_demo
  arena_all_tests_demo.cc)


