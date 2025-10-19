# In your CMakeLists.txt (or tests/CMakeLists.txt)
find_package(Catch2 3 REQUIRED)
enable_testing()

add_executable(navary-engine-test
  #memory/block_test.cc
  result_test.cc
  result_int_test.cc
  result_macro_test.cc

)

# target_include_directories(block_tests PRIVATE
#   ${CMAKE_SOURCE_DIR}/include       # so "navary/memory/block.hpp" resolves
# )

target_link_libraries(navary-engine-test PRIVATE Catch2::Catch2WithMain navary::engine)
add_test(NAME navary-engine-test COMMAND navary-engine-test)
