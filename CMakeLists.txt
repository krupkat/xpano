cmake_minimum_required(VERSION 3.15)
project(SDLTest)

set(CMAKE_CXX_STANDARD 20)
set(ABSL_PROPAGATE_CXX_STD ON)

IF (WIN32)
	IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(OpenCV_DIR "C:/Users/tomas/opencv/build/install_debug")
	ELSE()
		set(OpenCV_DIR "C:/Users/tomas/opencv/build/install")
	ENDIF()
	set(SDL2_DIR "C:/Users/tomas/SDL/build/install/cmake")
ENDIF()

add_subdirectory("external/abseil-cpp")
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

set (XPANO_SOURCES
	"xpano/main.cc"
	"xpano/algorithm.cc"
	"xpano/file_dialog.cc"
	"xpano/image.cc"
	"xpano/layout.cc"
	"xpano/logger.cc"
	"xpano/pano_gui.cc"
	"xpano/preview_pane.cc"
	"xpano/stitcher_pipeline.cc"
	"xpano/thumbnail_pane.cc"
	"xpano/utils/imgui_.cc"
)

find_package(SDL2 REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(SDLTest WIN32
	${XPANO_SOURCES}
	${IMGUI_SOURCES}
)

target_include_directories(SDLTest PRIVATE 
	"external/imgui" 
	"external/imgui/backends"
	"external/thread-pool")

target_link_libraries(SDLTest 
	absl::strings
	nfd
	${OpenCV_LIBRARIES}
	${SDL2_LIBRARIES}
)

install(TARGETS SDLTest)
