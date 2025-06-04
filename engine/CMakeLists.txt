cmake_minimum_required(VERSION 3.10)
project(navari)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(NVR_VERSION_HEADER "${CMAKE_SOURCE_DIR}/version.h")

set(NAVARI_VER "0.0.9")
message( STATUS "Navari Engine v${NAVARI_VER}")
message( STATUS "-----------------------------")


NV_GET_CXX_STD_FEATURE(${STX_CXX_VERSION} CXX_FEATURE)
message(STATUS "CXX Feature: ${CXX_FEATURE}")

option (NAVARI_ENGINE_NAME "navari")
set (NAVARI_ENGINE_NAME "navari")


# Ensure CMAKE_BUILD_TYPE is set correctly.
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()
string(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" BUILD_TYPE_CXX_FLAGS)


set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

#Main headers and sources
# file(GLOB_RECURSE SOURCES_STX CONFIGURE_DEPENDS
#     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
#     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
#     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.s
# )

include(${CMAKE_CURRENT_LIST_DIR}/src/navari/sources.cmake)

add_library(${PROJECT_NAME} STATIC ${NAVARI_SRC})
target_link_libraries(${PROJECT_NAME}  
    PUBLIC
        Vulkan::Vulkan
        fmt
        GPUOpen::VulkanMemoryAllocator
        vk-bootstrap::vk-bootstrap
        SDL2-static
        # glfw
        # glm::glm
        # assimp
        # imgui
        # fastgltf
        # EnTT::EnTT
    )
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(${PROJECT_NAME} PUBLIC "NAVARI_USE_LIBUV=0")
target_compile_features(${PROJECT_NAME} PUBLIC ${CXX_FEATURE})
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/fp16/
)

add_library(${PROJECT_NAME}::engine ALIAS ${PROJECT_NAME} )


#runner executable
add_executable(${PROJECT_NAME}-runner main.cc)
target_link_libraries(${PROJECT_NAME}-runner PUBLIC navari::engine)
set_target_properties(${PROJECT_NAME}-runner PROPERTIES LINKER_LANGUAGE CXX)
target_compile_features(${PROJECT_NAME}-runner PUBLIC ${CXX_FEATURE})

