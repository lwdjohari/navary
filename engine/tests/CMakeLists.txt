# In your CMakeLists.txt (or tests/CMakeLists.txt)
find_package(Catch2 3 REQUIRED)
enable_testing()

add_executable(navary-engine-test
  #memory/block_test.cc
  result_test.cc
  result_int_test.cc
  result_macro_test.cc
  math/vec2_test.cc
  math/vec3_test.cc
  math/vec4_test.cc
  math/rect_test.cc
  math/fixed_test.cc
  math/fixed_sim_test.cc
  math/fixed_vec2_test.cc
  math/fixed_vec3_test.cc
  math/fixed_vec3_rotate_test.cc
  math/math_util_test.cc
  math/plane_test.cc
  math/mat4_test.cc
  math/quat_test.cc
  math/camera_proj_test.cc
  math/camera_proj_vulkan_test.cc

)

# target_include_directories(block_tests PRIVATE
#   ${CMAKE_SOURCE_DIR}/include       # so "navary/memory/block.hpp" resolves
# )

target_link_libraries(navary-engine-test PRIVATE Catch2::Catch2WithMain navary::engine)
add_test(NAME navary-engine-test COMMAND navary-engine-test)
