cmake_minimum_required(VERSION 3.21)
OPTION(BUILD_TESTING "Build tests" OFF)

if(XPANO_STATIC_VCRT)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(Xpano)
include(CTest)
include("${CMAKE_SOURCE_DIR}/misc/cmake/utils.cmake")

if (MSVC)
  add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 20)
set(ABSL_PROPAGATE_CXX_STD ON)

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
  "xpano/gui/backends/base.cc"
  "xpano/gui/backends/sdl.cc"
  "xpano/gui/file_dialog.cc"
  "xpano/gui/layout.cc"
  "xpano/gui/panels/about.cc"
  "xpano/gui/panels/preview_pane.cc"
  "xpano/gui/panels/sidebar.cc"
  "xpano/gui/panels/thumbnail_pane.cc"
  "xpano/gui/pano_gui.cc"
  "xpano/utils/imgui_.cc"
  "xpano/utils/resource.cc"
  "xpano/utils/sdl_.cc"
  "xpano/utils/text.cc"
)

find_package(SDL2 REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS calib3d core features2d flann imgcodecs imgproc stitching)
find_package(spdlog REQUIRED)

add_executable(Xpano WIN32
  ${XPANO_SOURCES}
  ${IMGUI_SOURCES}
)

target_include_directories(Xpano PRIVATE 
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

target_link_libraries(Xpano
  nfd
  ${OPENCV_TARGETS}
  SDL2::SDL2
  SDL2::SDL2main
  spdlog::spdlog
)

copy_runtime_dlls(Xpano)
copy_directory(Xpano 
  "${CMAKE_SOURCE_DIR}/misc/fonts"
)

if (WIN32)
  set(CMAKE_INSTALL_BINDIR ".")
  set(CMAKE_INSTALL_DATADIR ".")
else()
  set(CMAKE_INSTALL_BINDIR "bin")
  set(CMAKE_INSTALL_DATADIR "share")
endif()

install(TARGETS
  Xpano RUNTIME
)

install(FILES 
  "$<TARGET_RUNTIME_DLLS:Xpano>" 
  TYPE BIN
)

install(DIRECTORY
  "${CMAKE_SOURCE_DIR}/misc/fonts"
  TYPE DATA
)

install(FILES
  "external/imgui/LICENSE.txt"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses"
  RENAME "imgui-license.txt"
)
install(FILES
  "external/nativefiledialog-extended/LICENSE"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses"
  RENAME "nativefiledialog-extended-license.txt"
)
install(FILES
  "external/thread-pool/LICENSE.txt"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses"
  RENAME "thread-pool-license.txt"
)
install(FILES
  "LICENSE"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses"
  RENAME "xpano-license.txt"
)
install(DIRECTORY
  "${OpenCV_DIR}/etc/licenses"
  TYPE DATA
  OPTIONAL
)
install(FILES
  "${OpenCV_DIR}/LICENSE"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses"
  RENAME "opencv-license.txt"
  OPTIONAL
)
if(DEFINED XPANO_EXTRA_LICENSES)
  install(DIRECTORY
    "${XPANO_EXTRA_LICENSES}"
    TYPE DATA
  )
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
