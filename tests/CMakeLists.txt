cmake_minimum_required(VERSION 3.15)

find_package(Catch2 REQUIRED)
include(Catch)
include("${CMAKE_SOURCE_DIR}/misc/cmake/utils.cmake")

add_executable(AutoCropTest 
  auto_crop_test.cc
  ../xpano/algorithm/auto_crop.cc)

target_link_libraries(AutoCropTest 
  Catch2::Catch2WithMain
  ${OPENCV_TARGETS}
  spdlog::spdlog
)

target_include_directories(AutoCropTest PRIVATE 
  ".."
)

add_executable(StitcherTest 
  stitcher_pipeline_test.cc
  ../xpano/algorithm/algorithm.cc
  ../xpano/algorithm/auto_crop.cc
  ../xpano/algorithm/image.cc
  ../xpano/pipeline/stitcher_pipeline.cc
  ../xpano/utils/disjoint_set.cc)

target_link_libraries(StitcherTest 
  Catch2::Catch2WithMain
  ${OPENCV_TARGETS}
  spdlog::spdlog
)

target_include_directories(StitcherTest PRIVATE 
  ".."
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
  ".."
)

add_executable(RectTest 
  rect_test.cc
)

target_link_libraries(RectTest 
  Catch2::Catch2WithMain
)

target_include_directories(RectTest PRIVATE 
  ".."
)

add_executable(DisjointSetTest 
  disjoint_set_test.cc
  ../xpano/utils/disjoint_set.cc
)

target_link_libraries(DisjointSetTest 
  Catch2::Catch2WithMain
)

target_include_directories(DisjointSetTest PRIVATE 
  ".."
)

set(ALL_TEST_TARGETS
  AutoCropTest
  DisjointSetTest
  RectTest
  StitcherTest
  VecTest  
)

foreach(name ${ALL_TEST_TARGETS})
  copy_runtime_dlls(${name})
  catch_discover_tests(${name} 
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:${name}>"
  )
endforeach()
