cmake_minimum_required(VERSION 3.15)

find_package(Catch2 REQUIRED)
include(Catch)

add_executable(VecTest vec_test.cc)

set(ALL_TEST_TARGETS
  VecTest
)

foreach( name ${ALL_TEST_TARGETS} )
  target_link_libraries(${name} Catch2::Catch2WithMain)
  target_include_directories(${name} PRIVATE "../xpano")
  catch_discover_tests(${name})
endforeach()
