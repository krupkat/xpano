cmake_minimum_required(VERSION 3.15)

find_package(Catch2 REQUIRED)
include(Catch)
include("${CMAKE_SOURCE_DIR}/misc/cmake/utils.cmake")

add_executable(StitcherTest 
  stitcher_test.cc
  ../xpano/algorithm/algorithm.cc
  ../xpano/algorithm/image.cc
  ../xpano/algorithm/stitcher_pipeline.cc)

target_link_libraries(StitcherTest 
  Catch2::Catch2WithMain
  ${OPENCV_TARGETS}
)

target_include_directories(StitcherTest PRIVATE 
  "../xpano"
  "../external/thread-pool"
)

copy_directory(StitcherTest ${CMAKE_CURRENT_SOURCE_DIR}/data)

add_executable(VecTest 
  vec_test.cc
)

target_link_libraries(VecTest 
  Catch2::Catch2WithMain
)

target_include_directories(VecTest PRIVATE 
  "../xpano"
)

set(ALL_TEST_TARGETS
  StitcherTest
  VecTest
)

foreach( name ${ALL_TEST_TARGETS} )
  copy_runtime_dlls(${name})
  catch_discover_tests(${name})  
endforeach()
