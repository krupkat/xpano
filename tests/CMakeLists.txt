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

copy_file(AutoCropTest ${CMAKE_CURRENT_SOURCE_DIR}/data/mask.png)

add_executable(StitcherTest 
  stitcher_pipeline_test.cc
  ../xpano/algorithm/algorithm.cc
  ../xpano/algorithm/auto_crop.cc
  ../xpano/algorithm/blenders.cc
  ../xpano/algorithm/image.cc
  ../xpano/algorithm/stitcher.cc
  ../xpano/pipeline/options.cc
  ../xpano/pipeline/stitcher_pipeline.cc
  ../xpano/utils/disjoint_set.cc
  ../xpano/utils/exiv2.cc
  ../xpano/utils/path.cc)

target_link_libraries(StitcherTest 
  Catch2::Catch2WithMain
  ${OPENCV_TARGETS}
  spdlog::spdlog
)

if(exiv2_FOUND)
  target_compile_definitions(StitcherTest PRIVATE XPANO_WITH_EXIV2)
  target_link_libraries(StitcherTest exiv2lib)
endif()

if(XPANO_WITH_MULTIBLEND)
  target_compile_definitions(StitcherTest PRIVATE XPANO_WITH_MULTIBLEND)
  target_link_libraries(StitcherTest MultiblendLib)
endif()

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

add_executable(SerializeTest 
  serialize_test.cc
  ../xpano/algorithm/options.cc
  ../xpano/pipeline/options.cc
)

target_link_libraries(SerializeTest 
  Catch2::Catch2WithMain
  alpaca
  spdlog::spdlog
)

target_include_directories(SerializeTest PRIVATE 
  ".."
)

add_executable(ArgsTest 
  args_test.cc
  ../xpano/cli/args.cc
  ../xpano/utils/path.cc
)

target_link_libraries(ArgsTest 
  Catch2::Catch2WithMain
  spdlog::spdlog
)

target_include_directories(ArgsTest PRIVATE 
  ".."
)

set(ALL_TEST_TARGETS
  AutoCropTest
  DisjointSetTest
  RectTest
  StitcherTest
  VecTest
  SerializeTest
  ArgsTest
)

foreach(name ${ALL_TEST_TARGETS})
  copy_runtime_dlls(${name})
  catch_discover_tests(${name} 
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:${name}>"
  )
endforeach()
