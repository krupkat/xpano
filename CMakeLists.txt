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

add_subdirectory("external/nativefiledialog-extended")
add_subdirectory("external/alpaca" EXCLUDE_FROM_ALL)

set(EXPECTED_BUILD_TESTS OFF) # needed to disable download of catch2
add_subdirectory("external/expected" EXCLUDE_FROM_ALL)

set(IMGUI_SOURCES 
  "external/imgui/imgui.cpp"
  "external/imgui/imgui_draw.cpp"
  "external/imgui/imgui_demo.cpp"
  "external/imgui/imgui_tables.cpp"
  "external/imgui/imgui_widgets.cpp"
  "external/imgui/backends/imgui_impl_sdl2.cpp"
  "external/imgui/backends/imgui_impl_sdlrenderer.cpp"
)

set(XPANO_SOURCES
  "xpano/main.cc"
  "xpano/algorithm/algorithm.cc"
  "xpano/algorithm/auto_crop.cc"
  "xpano/algorithm/bundle_adjuster.cc"
  "xpano/algorithm/image.cc"
  "xpano/algorithm/options.cc"
  "xpano/cli/args.cc"
  "xpano/cli/pano_cli.cc"
  "xpano/cli/signal.cc"
  "xpano/log/logger.cc"
  "xpano/gui/backends/base.cc"
  "xpano/gui/backends/sdl.cc"
  "xpano/gui/file_dialog.cc"
  "xpano/gui/layout.cc"
  "xpano/gui/panels/about.cc"
  "xpano/gui/panels/bugreport_pane.cc"
  "xpano/gui/panels/log_pane.cc"
  "xpano/gui/panels/preview_pane.cc"
  "xpano/gui/panels/sidebar.cc"
  "xpano/gui/panels/thumbnail_pane.cc"
  "xpano/gui/panels/warning_pane.cc"
  "xpano/gui/pano_gui.cc"
  "xpano/gui/shortcut.cc"
  "xpano/pipeline/options.cc"
  "xpano/pipeline/stitcher_pipeline.cc"
  "xpano/utils/config.cc"
  "xpano/utils/disjoint_set.cc"
  "xpano/utils/exiv2.cc"
  "xpano/utils/imgui_.cc"
  "xpano/utils/path.cc"
  "xpano/utils/resource.cc"
  "xpano/utils/sdl_.cc"
  "xpano/utils/text.cc"
)

if (WIN32)
 list(APPEND XPANO_SOURCES "xpano/cli/windows_console.cc")
endif()

find_package(SDL2 REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS calib3d core features2d flann imgcodecs imgproc photo stitching)
find_package(spdlog REQUIRED)
find_package(exiv2 REQUIRED)

add_executable(Xpano WIN32
  ${XPANO_SOURCES}
  ${IMGUI_SOURCES}
)

target_include_directories(Xpano PRIVATE 
  "external/imgui" 
  "external/imgui/backends"
  "external/thread-pool"
  "."
)

set(OPENCV_TARGETS
  opencv_calib3d
  opencv_core
  opencv_features2d
  opencv_flann
  opencv_imgcodecs
  opencv_imgproc
  opencv_photo
  opencv_stitching
)

if(NOT TARGET SDL2::SDL2main)
# This is a workaround for SDL 2.24.0, still needed for build on Kinetic
  add_library(SDL2::SDL2main INTERFACE IMPORTED)
endif()

target_link_libraries(Xpano
  alpaca
  expected
  nfd
  ${OPENCV_TARGETS}
  SDL2::SDL2
  SDL2::SDL2main
  spdlog::spdlog
  exiv2lib
)

if(NOT MSVC)
  target_link_options(Xpano
    PRIVATE
    "LINKER:-v,--trace"
  )
endif()

copy_runtime_dlls(Xpano)
copy_directory(Xpano 
  "${CMAKE_SOURCE_DIR}/misc/assets"
)

if (WIN32)
  set(CMAKE_INSTALL_BINDIR ".")
  set(CMAKE_INSTALL_DATADIR ".")
else()
  set(CMAKE_INSTALL_BINDIR "bin")
  set(CMAKE_INSTALL_DATADIR "share/xpano")
endif()

install(TARGETS
  Xpano RUNTIME
)

install(FILES 
  "$<TARGET_RUNTIME_DLLS:Xpano>" 
  TYPE BIN
)

if(CMAKE_INSTALL_PREFIX MATCHES "^/usr.*|^/app.*")
  install(FILES
    "misc/build/linux/xpano.desktop"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications"
  )
  install(FILES
    "misc/build/linux/xpano.png"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/share/pixmaps"
  )
  install(FILES
    "misc/build/linux/xpano.svg"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps"
  )
  install(FILES
    "misc/build/linux/cz.krupkat.Xpano.metainfo.xml"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/share/metainfo"
  )
endif()

install(DIRECTORY
  "${CMAKE_SOURCE_DIR}/misc/assets"
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
  "external/alpaca/LICENSE"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses"
  RENAME "alpaca-license.txt"
)
install(FILES
  "misc/assets/OFL.txt"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses"
  RENAME "google-noto-license.txt"
)
install(FILES
  "LICENSE"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses"
  RENAME "xpano-license.txt"
)
install(FILES
  "CHANGELOG.md"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses"
)
install(FILES
  "misc/build/opencv-notice.txt"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses"
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
install(FILES
  "misc/build/exiv2-notice.txt"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses"
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
