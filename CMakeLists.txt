cmake_minimum_required(VERSION 3.21)
OPTION(BUILD_TESTING "Build tests" OFF)

project(SDLTest)
include(CTest)
include("${CMAKE_SOURCE_DIR}/misc/cmake/utils.cmake")

set(CMAKE_CXX_STANDARD 20)
set(ABSL_PROPAGATE_CXX_STD ON)

add_subdirectory("external/abseil-cpp" EXCLUDE_FROM_ALL)
add_subdirectory("external/nativefiledialog-extended" EXCLUDE_FROM_ALL)

set(IMGUI_SOURCES 
  "external/imgui/imgui.cpp"
  "external/imgui/imgui_draw.cpp"
  "external/imgui/imgui_demo.cpp"
  "external/imgui/imgui_tables.cpp"
  "external/imgui/imgui_widgets.cpp"
  "external/imgui/backends/imgui_impl_sdl.cpp"
  "external/imgui/backends/imgui_impl_sdlrenderer.cpp"
)

set(XPANO_SOURCES
  "xpano/main.cc"
  "xpano/algorithm/algorithm.cc"
  "xpano/algorithm/image.cc"
  "xpano/algorithm/stitcher_pipeline.cc"
  "xpano/log/logger.cc"
  "xpano/gui/file_dialog.cc"  
  "xpano/gui/layout.cc"  
  "xpano/gui/pano_gui.cc"
  "xpano/gui/preview_pane.cc"  
  "xpano/gui/thumbnail_pane.cc"
  "xpano/utils/imgui_.cc"
)

find_package(SDL2 REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS calib3d core features2d flann imgcodecs imgproc stitching)

add_executable(SDLTest WIN32
  ${XPANO_SOURCES}
  ${IMGUI_SOURCES}
)

target_include_directories(SDLTest PRIVATE 
  "external/imgui" 
  "external/imgui/backends"
  "external/thread-pool"
  "xpano"
)

set(OPENCV_TARGETS
  opencv_calib3d
  opencv_core
  opencv_features2d
  opencv_flann
  opencv_imgcodecs
  opencv_imgproc
  opencv_stitching
)

target_link_libraries(SDLTest
  absl::strings
  nfd
  ${OPENCV_TARGETS}
  SDL2::SDL2
  SDL2::SDL2main
)

copy_runtime_dlls(SDLTest)
copy_file(SDLTest "${CMAKE_SOURCE_DIR}/external/imgui/misc/fonts/DroidSans.ttf")

install(TARGETS
  SDLTest
)

install(FILES 
  "$<TARGET_RUNTIME_DLLS:SDLTest>" 
  "${CMAKE_SOURCE_DIR}/external/imgui/misc/fonts/DroidSans.ttf"
  TYPE BIN
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
