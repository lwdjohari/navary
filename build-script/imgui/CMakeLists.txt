cmake_minimum_required(VERSION 3.10)

# Create an interface library for non-docked ImGui
set(IMGUI_CORE_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/imgui-docked/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/imgui-docked/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/imgui-docked/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/imgui-docked/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/imgui-docked/imgui_tables.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/imgui/misc/cpp/imgui_stdlib.cpp
)

# Backend source files
set(IMGUI_BACKEND_VULKAN_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/imgui-docked/backends/imgui_impl_sdl2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/imgui-docked//backends/imgui_impl_vulkan.cpp
)

if(NVR_USE_WGPU)
set(IMGUI_BACKEND_WGPU_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/imgui-docked/backends/imgui_impl_wgpu.cpp
)
endif()

# Create an interface library for non-docked ImGui with Vulkan
add_library(imgui STATIC ${IMGUI_CORE_SRC} ${IMGUI_BACKEND_VULKAN_SRC})

target_link_libraries(imgui PUBLIC SDL2-static)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/imgui-docked/
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/imgui-docked/backends/
)

target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_DOCKING)

# Link dependencies for non-docked ImGui
# target_link_libraries(imgui PUBLIC
#     SDL2::SDL2
#     Vulkan::Vulkan
# )
