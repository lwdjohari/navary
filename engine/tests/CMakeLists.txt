# In your CMakeLists.txt (or tests/CMakeLists.txt)
find_package(Catch2 3 REQUIRED)
enable_testing()

add_executable(navary-core-test
  result_test.cc
  result_int_test.cc
  result_macro_test.cc
  memory/arena_test.cc
  memory/arena_complex_test.cc
)

add_executable(navary-math-test
  math/vec2_test.cc
  math/vec3_test.cc
  math/vec4_test.cc
  math/rect_test.cc
  math/math_util_test.cc
  math/plane_test.cc
)

add_executable(navary-matquat-test
  math/mat4_test.cc
  math/quat_test.cc
  math/camera_proj_test.cc
  math/camera_proj_vulkan_test.cc
)

add_executable(navary-frustbb-test
  math/frustum_test.cc
  math/aabb_test.cc
  math/obb_test.cc

)

# target_include_directories(block_tests PRIVATE
#   ${CMAKE_SOURCE_DIR}/include       # so "navary/memory/block.hpp" resolves
# )

target_link_libraries(navary-core-test PRIVATE Catch2::Catch2WithMain navary::engine)
add_test(NAME navary-core-test COMMAND navary-core-test)

target_link_libraries(navary-math-test PRIVATE Catch2::Catch2WithMain navary::engine)
add_test(NAME navary-math-test COMMAND navary-math-test)

target_link_libraries(navary-matquat-test PRIVATE Catch2::Catch2WithMain navary::engine)
add_test(NAME navary-matquat-test COMMAND navary-matquat-test)

target_link_libraries(navary-frustbb-test PRIVATE Catch2::Catch2WithMain navary::engine)
add_test(NAME navary-frustbb-test COMMAND navary-frustbb-test)

