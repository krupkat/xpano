// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Vaibhav Sharma
// SPDX-License-Identifier: GPL-3.0-or-later

#include <clocale>
#include <cstdio>
#include <future>
#include <string>
#include <utility>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <nfd.h>
#include <SDL.h>
#include <spdlog/spdlog.h>

#include "xpano/cli/pano_cli.h"
#include "xpano/constants.h"
#include "xpano/gui/backends/sdl.h"
#include "xpano/gui/pano_gui.h"
#include "xpano/log/logger.h"
#include "xpano/utils/config.h"
#include "xpano/utils/fmt.h"
#include "xpano/utils/imgui_.h"
#include "xpano/utils/resource.h"
#include "xpano/utils/sdl_.h"
#include "xpano/utils/text.h"
#include "xpano/version_fmt.h"

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

int main(int argc, char** argv) {
  const char* locale = std::setlocale(LC_ALL, "en_US.UTF-8");
  auto [cli_status, args] = xpano::cli::Run(argc, argv);

  if (cli_status != xpano::cli::ResultType::kForwardToGui) {
    return xpano::cli::ExitCode(cli_status);
  }

#if SDL_VERSION_ATLEAST(2, 23, 1)
  SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
  // This feature isn't compatible with ImGui as of v1.88
  // SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
#endif

  const bool has_wayland_support = (SDL_VideoInit("wayland") == 0);

#if SDL_VERSION_ATLEAST(2, 0, 22)
  // Prefer Wayland as it provides non-blurry fractional scaling
  if (has_wayland_support) {
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland,x11");
  }
#endif

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    printf("Error: %s\n", SDL_GetError());
    return -1;
  }

  auto app_data_path = xpano::utils::sdl::InitializePrefPath();
  auto app_exe_path = xpano::utils::sdl::InitializeBasePath();

  // Setup logging
  xpano::logger::Logger logger{};
  logger.RedirectSpdlogToGui(app_data_path);
  xpano::logger::RedirectSDLOutput();
  if (locale != nullptr) {
    spdlog::info("Current locale: {}", locale);
  }

  if (!app_data_path) {
    spdlog::warn(
        "Failed to initialize application data path, skipping logging to "
        "files.");
  }

  if (!app_exe_path) {
    spdlog::error(
        "Failed to initialize application executable path, shutting down.");
    return -1;
  }

  auto config = xpano::utils::config::Load(app_data_path);

  // Setup file dialog library
  if (NFD_Init() != NFD_OKAY) {
    spdlog::error("Couldn't initialize NFD");
  }

  // Setup SDL Window + Renderer
  auto window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
  auto window_title = fmt::format("Xpano {}", xpano::version::Current());
  SDL_Window* window =
      SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, config.app_state.window_width,
                       config.app_state.window_height, window_flags);

  if (window == nullptr) {
    spdlog::error("Error creating SDL_Window! {}", SDL_GetError());
    return -1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
  if (renderer == nullptr) {
    spdlog::error("Error creating SDL_Renderer! {}", SDL_GetError());
    return -1;
  }

  auto icon = xpano::utils::resource::LoadIcon(*app_exe_path, xpano::kIconPath);
  SDL_SetWindowIcon(window, icon.get());

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& imgui_io = ImGui::GetIO();
  auto imgui_ini_file = xpano::utils::imgui::InitIniFilePath(app_data_path);
  imgui_io.IniFilename = app_data_path ? imgui_ini_file.c_str() : nullptr;
  imgui_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);

  const SDL_Color clear_color{114, 140, 165, 255};

  // Application specific
  auto backend = xpano::gui::backends::Sdl{renderer};

  std::future<xpano::utils::Texts> license_texts =
      std::async(std::launch::async, xpano::utils::LoadTexts, *app_exe_path,
                 xpano::kLicensePath);

  xpano::gui::PanoGui gui(&backend, &logger, config, std::move(license_texts),
                          *args);

  auto window_manager =
      xpano::utils::sdl::DetermineWindowManager(has_wayland_support);
  xpano::utils::sdl::DpiHandler dpi_handler(window, window_manager);
  xpano::utils::imgui::FontLoader font_loader(xpano::kFontPath,
                                              xpano::kSymbolsFontPath);
  if (!font_loader.Init(*app_exe_path)) {
    spdlog::error("Font location not found!");
    return -1;
  }

  // Main loop
  bool done = false;
  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event) > 0) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT) {
        done = true;
      }
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window)) {
        done = true;
      }
    }

    // Handle DPI change
    if (dpi_handler.DpiChanged()) {
      font_loader.Reload(dpi_handler.DpiScale());
    }

    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // User code
    done |= gui.Run();

    // ImGui::ShowDemoWindow();

    // Rendering
    SDL_SetRenderDrawColor(renderer, clear_color.r, clear_color.g,
                           clear_color.b, clear_color.a);
    SDL_RenderClear(renderer);
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
  }

  auto size = xpano::utils::sdl::GetSize(window);
  xpano::utils::config::Save(app_data_path, size, gui.GetOptions());

  // Cleanup
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  NFD_Quit();
  SDL_Quit();

  return 0;
}
