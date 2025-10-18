cmake_minimum_required(VERSION 3.10)
project(navary)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(NAVARY__VER "0.0.9")

set(SDL_SHARED OFF)
set(SDL2_DISABLE_INSTALL ON)
set(BUILD_SHARED_LIBS OFF)
# set(GLFW_BUILD_EXAMPLES  ${GLFW_STANDALONE})
# set(GLFW_BUILD_TESTS  ${GLFW_STANDALONE})
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
set(GLM_BUILD_INSTALL OFF)
set(GLM_ENABLE_CXX_17 ON)
set(GLM_ENABLE_FAST_MATH ON)
set(GLM_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INJECT_DEBUG_POSTFIX ON)
set(MESHOPT_BUILD_SHARED_LIBS OFF)
set(FASTGLTF_COMPILE_AS_CPP20 OFF)


set(BUILD_SHARED_LIBS OFF)

# Assimp
set(ASSIMP_INSTALL OFF)  
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INJECT_DEBUG_POSTFIX ON)
# GLM
set(GLM_BUILD_INSTALL OFF)
set(GLM_ENABLE_CXX_17 ON)
set(GLM_ENABLE_FAST_MATH ON)
set(GLM_BUILD_TESTS OFF)

# option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" ${GLFW_STANDALONE})
# option(GLFW_BUILD_TESTS "Build the GLFW test programs" ${GLFW_STANDALONE})
set(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
set(GLFW_INSTALL "Generate installation target" OFF)

include(ProjectCXX)
set(ISROOT FALSE)
set(CXX_FEATURE "")
NV_DETECT_IS_ROOT_PROJECT(ISROOT)

if(ISROOT)
  if(NOT STX_CXX_VERSION)
    set(STX_CXX_VERSION 20)
  endif()
    option(STX_CXX_STANDARD_REQUIRED "CXX Required" ON)
    option(STX_CXX_EXTENSIONS "CXX Extensions" OFF)
    
  NV_PROPOGATE_CXX_OPTIONS(${STX_CXX_VERSION} ${STX_CXX_STANDARD_REQUIRED} ${STX_CXX_EXTENSIONS})
  set(STX_USE_CATCH ON)
  set(STX_USE_TEST ON)
  set(STX_BUILD_EXAMPLE ON)
  set(CXX_FEATURE "")
else()
    set(STX_CXX_VERSION ${CMAKE_CXX_STANDARD})
endif()

message(STATUS "")

include(ProjectCXX)
set(ISROOT FALSE)
NV_DETECT_IS_ROOT_PROJECT(ISROOT)
NV_GET_CXX_STD_FEATURE(${STX_CXX_VERSION} CXX_FEATURE)

find_package(Threads REQUIRED)

add_subdirectory(thirdparty/fmt build-fmt)

# find Vulkan lib
find_package(Vulkan REQUIRED)

# Add SDL2
add_subdirectory(thirdparty/sdl build-sdl)
# Detect SDL2 include directory
if(TARGET SDL2::SDL2)
  get_target_property(_SDL_INCLUDE SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
  if(_SDL_INCLUDE)
    set(SDL2_INCLUDE_DIRS ${_SDL_INCLUDE})
  endif()
elseif(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/sdl/include")
  set(SDL2_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/thirdparty/sdl/include")
endif()

# add vma
add_subdirectory(thirdparty/vma build-vma)

# add vk-bootstrap
add_subdirectory(thirdparty/vk-bootstrap build-vk-bootstrap)

# add imgui
# add_subdirectory(build-script/imgui build-imgui)

# add GLFW
# add_subdirectory(thirdparty/glfw build-glfw)

# add GLM
# add_subdirectory(thirdparty/glm build-glm)

# add Assimp
# add_subdirectory(thirdparty/assimp build-assimp)

# add Meshoptimizer
# add_subdirectory(thirdparty/meshoptimizer build-meshoptimizer)

# add fastgltf
# add_subdirectory(thirdparty/fastgltf build-fastgltf)

# add entt
add_library(entt INTERFACE)
target_include_directories(entt INTERFACE
    ${CMAKE_SOURCE_DIR}/thirdparty/entt/src
)
add_library(entt::entt ALIAS entt)


# Include directories
add_subdirectory(engine build-engine)
# add_subdirectory(editor build-editor)

message(STATUS "")
message(STATUS "----------------------------------------------")
message(STATUS "  Navary Engine Project Info")
message(STATUS "----------------------------------------------")
message(STATUS "C++ Standard:         ${STX_CXX_VERSION}")
message(STATUS "CXX Feature:          ${CXX_FEATURE}")
message(STATUS "Root Project:         ${ISROOT}")
message(STATUS "Source Dir:           ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Build Dir:            ${CMAKE_BINARY_DIR}")
message(STATUS "Version:              ${NAVARY__VER}")
message(STATUS "")
message(STATUS "----------------------------------------------")
message(STATUS "  Navary Engine Deps")
message(STATUS "----------------------------------------------")

# --- Vulkan ---
if(Vulkan_FOUND)
  message(STATUS "Vulkan:               FOUND")
else()
  message(STATUS "Vulkan:               NOT FOUND")
endif()

# --- SDL2 ---
if(TARGET SDL2::SDL2)
  message(STATUS "SDL2:                 ENABLED (build-sdl)")
else()
  message(STATUS "SDL2:                 DISABLED / Not built")
endif()

# --- fmt ---
if(TARGET fmt::fmt)
  message(STATUS "fmt:                  ENABLED (build-fmt)")
else()
  message(STATUS "fmt:                  DISABLED / Not built")
endif()

# --- Vulkan Memory Allocator (VMA) ---
if(TARGET VulkanMemoryAllocator)
  message(STATUS "VMA:                  ENABLED (build-vma)")
else()
  message(STATUS "VMA:                  DISABLED / Not built")
endif()

# --- vk-bootstrap ---
if(TARGET vk-bootstrap::vk-bootstrap)
  message(STATUS "vk-bootstrap:         ENABLED (build-vk-bootstrap)")
else()
  message(STATUS "vk-bootstrap:         DISABLED / Not built")
endif()

# --- GLM ---
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/glm/CMakeLists.txt")
  message(STATUS "GLM:                  SOURCE PRESENT (build disabled here)")
else()
  message(STATUS "GLM:                  NOT PRESENT")
endif()

# --- Assimp ---
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/assimp/CMakeLists.txt")
  message(STATUS "Assimp:               SOURCE PRESENT (build disabled here)")
else()
  message(STATUS "Assimp:               NOT PRESENT")
endif()

# --- MeshOptimizer ---
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/meshoptimizer/CMakeLists.txt")
  message(STATUS "MeshOptimizer:        SOURCE PRESENT (build disabled here)")
else()
  message(STATUS "MeshOptimizer:        NOT PRESENT")
endif()

# --- fastgltf ---
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/fastgltf/CMakeLists.txt")
  message(STATUS "fastgltf:             SOURCE PRESENT (build disabled here)")
else()
  message(STATUS "fastgltf:             NOT PRESENT")
endif()

# --- EnTT ---
if(TARGET entt)
  message(STATUS "EnTT:                 ENABLED (build-entt)")
else()
  message(STATUS "EnTT:                 DISABLED / Not built")
endif()

message(STATUS "")
message(STATUS "----------------------------------------------")
message(STATUS "  Navary Include Dir")
message(STATUS "----------------------------------------------")

# --- SDL2 ---
if(TARGET SDL2::SDL2)
  message(STATUS "SDL2:")
  message(STATUS "${SDL2_INCLUDE_DIRS}")
endif()

message(STATUS "")