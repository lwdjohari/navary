cmake_minimum_required(VERSION 3.10)
project(stratengine)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(NAVARY__VER "0.0.9")
message( STATUS "Navary v${NAVARY__VER}")
message( STATUS "-----------------------------")

set(SDL_SHARED OFF)
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

option(BUILD_SHARED_LIBS OFF)
# Assimp
option(ASSIMP_INSTALL OFF)  
option(ASSIMP_BUILD_TESTS OFF)
option(ASSIMP_INJECT_DEBUG_POSTFIX ON)
# GLM
option(GLM_BUILD_INSTALL OFF)
option(GLM_ENABLE_CXX_17 ON)
option(GLM_ENABLE_FAST_MATH ON)
option(GLM_BUILD_TESTS OFF)

# option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" ${GLFW_STANDALONE})
# option(GLFW_BUILD_TESTS "Build the GLFW test programs" ${GLFW_STANDALONE})
option(GLFW_BUILD_DOCS "Build the GLFW documentation" ON)
option(GLFW_INSTALL "Generate installation target" ON)

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

message(STATUS "Navary LIB DIR:${CMAKE_CURRENT_SOURCE_DIR}")


include(ProjectCXX)
set(ISROOT FALSE)
NV_DETECT_IS_ROOT_PROJECT(ISROOT)
message(STATUS "Navary Root Project: ${ISROOT} ${STX_CXX_VERSION}")

NV_GET_CXX_STD_FEATURE(${STX_CXX_VERSION} CXX_FEATURE)
message(STATUS "CXX Feature: ${CXX_FEATURE}")

find_package(Threads REQUIRED)

#add FMT
message(STATUS "")
message(STATUS "Add libraries")
message(STATUS "---------------------")
message(STATUS "vulkan-sdk: ON")
message(STATUS "vulkan-memory-allocator: ON")
message(STATUS "vk-bootstrap: ON")
message(STATUS "FMT: ON")
message(STATUS "SDL2: ON")
message(STATUS "ImGui: ON")
message(STATUS "glm: ON")
message(STATUS "glfw: ON")
message(STATUS "assimp: ON")
message(STATUS "fastgltf: ON")
message(STATUS "EnTT: ON")
message(STATUS "")



add_subdirectory(thirdparty/fmt build-fmt)

# find Vulkan lib
find_package(Vulkan REQUIRED)

# Add SDL2
add_subdirectory(thirdparty/sdl build-sdl)
message(STATUS "SDL2 Include DIR: ${SDL2_INCLUDE_DIRS}")

#add vk-bootstrap
add_subdirectory(thirdparty/vma build-vma)

#add vk-bootstrap
add_subdirectory(thirdparty/vk-bootstrap build-vk-bootstrap)

# #add imgui
# add_subdirectory(build-script/imgui build-imgui)

# #add GLFW
# add_subdirectory(thirdparty/glfw build-glfw)

# #add GLM
# add_subdirectory(thirdparty/glm build-glm)

# #add Assimp
# add_subdirectory(thirdparty/assimp build-assimp)

# #add Meshoptimizer
# add_subdirectory(thirdparty/meshoptimizer build-meshoptimizer)

# #add fastgltf
# add_subdirectory(thirdparty/fastgltf build-fastgltf)

# #add entt
# add_subdirectory(thirdparty/entt build-entt)


# Include directories
add_subdirectory(engine build-engine)
# add_subdirectory(editor build-editor)




