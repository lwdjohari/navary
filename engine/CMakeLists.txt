cmake_minimum_required(VERSION 3.10)
project(navary)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(NVR_VERSION_HEADER "${CMAKE_SOURCE_DIR}/version.h")
set(NAVARI_VER "0.0.9")


NV_GET_CXX_STD_FEATURE(${STX_CXX_VERSION} CXX_FEATURE)
option (NAVARI_ENGINE_NAME "navary")
set (NAVARI_ENGINE_NAME "navary")


# Ensure CMAKE_BUILD_TYPE is set correctly.
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()
string(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" BUILD_TYPE_CXX_FLAGS)


set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)


# -------------------------------
# 1) Sources: list them explicitly
# -------------------------------
set(NAVARY_ENGINE_SOURCES
#    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/memory/arena_allocator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/vec2.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/vec3.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/vec4.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/rect.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/plane.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/fixed_trigonometry.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/fixed_vec2.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/fixed_vec3.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/mat4.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/quat.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/renderer/vulkan_render_backend.cc

)

# (Optional) If you plan to "install" public headers later, list them here.
# NOTE: These do NOT need to be in add_library() for compilation;
#       we keep a separate list just to use in install(FILES ...).
set(NAVARY_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/macro.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/size2.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/vec2.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/vec3.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/vec4.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/rect.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/fixed.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/math_util.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/plane.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/fixed_trigonometry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/fixed_vec2.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/fixed_vec3.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/mat4.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/math/quat.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/utility/random.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/memory/block.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/memory/arena_allocator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/platforms/platform.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/renderer/render_backend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/renderer/vulkan_render_backend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/navary/navary.h
)



add_library(${PROJECT_NAME} STATIC ${NAVARY_ENGINE_SOURCES})
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
        entt::entt
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
target_link_libraries(${PROJECT_NAME}-runner PUBLIC navary::engine)
set_target_properties(${PROJECT_NAME}-runner PROPERTIES LINKER_LANGUAGE CXX)
target_compile_features(${PROJECT_NAME}-runner PUBLIC ${CXX_FEATURE})

if(NAVARY_ENGINE_BUILD_TEST)
     message(STATUS "Navary Engine tests enabled (NAVARY_ENGINE_BUILD_TEST=ON)")
    add_subdirectory(tests build-engine-tests)
endif()
