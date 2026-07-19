// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Vaibhav Sharma
// SPDX-License-Identifier: GPL-3.0-or-later

#include <clocale>
#include <cstdio>
#include <future>
#include <string>
#include <utility>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <nfd.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
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

int main(int argc, char** argv) {
  const char* locale = std::setlocale(LC_ALL, "en_US.UTF-8");
  auto [cli_status, args] = xpano::cli::Run(argc, argv);

  if (cli_status != xpano::cli::ResultType::kForwardToGui) {
    return xpano::cli::ExitCode(cli_status);
  }

#ifdef __linux__
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");
#endif

  if (!SDL_Init(SDL_INIT_VIDEO)) {
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
  auto window_flags =
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  auto window_title = fmt::format("Xpano {}", xpano::version::Current());
  SDL_Window* window =
      SDL_CreateWindow(window_title.c_str(), config.app_state.window_width,
                       config.app_state.window_height, window_flags);

  if (window == nullptr) {
    spdlog::error("Error creating SDL_Window! {}", SDL_GetError());
    return -1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
  SDL_SetRenderVSync(renderer, 1);
  if (renderer == nullptr) {
    spdlog::error("Error creating SDL_Renderer! {}", SDL_GetError());
    return -1;
  }

  auto icon = xpano::utils::resource::LoadIcon(*app_exe_path, xpano::kIconPath);
  SDL_SetWindowIcon(window, icon.get());
  SDL_ShowWindow(window);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& imgui_io = ImGui::GetIO();
  auto imgui_ini_file = xpano::utils::imgui::InitIniFilePath(app_data_path);
  imgui_io.IniFilename = app_data_path ? imgui_ini_file.c_str() : nullptr;
  imgui_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  // Setup Platform/Renderer backends
  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  const SDL_Color clear_color{114, 140, 165, 255};

  // Application specific
  auto backend = xpano::gui::backends::Sdl{renderer};

  std::future<xpano::utils::Texts> license_texts =
      std::async(std::launch::async, xpano::utils::LoadTexts, *app_exe_path,
                 xpano::kLicensePath);

  xpano::gui::PanoGui gui(&backend, &logger, config, std::move(license_texts),
                          *args);

  if (!xpano::utils::imgui::LoadFonts(
          *app_exe_path, {.alphabet_font_path = xpano::kFontPath,
                          .symbols_font_path = xpano::kSymbolsFontPath})) {
    spdlog::error("Fonts could not be initialized!");
    return -1;
  }

  // Main loop
  bool done = false;
  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_QUIT ||
          (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
           event.window.windowID == SDL_GetWindowID(window))) {
        done = true;
      } else if (event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED) {
        xpano::utils::imgui::SetScale(xpano::utils::sdl::GetDpiScale(window));
      }
    }

    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // User code
    done |= gui.Run();

    // Rendering
    ImGui::Render();
    SDL_SetRenderScale(renderer, imgui_io.DisplayFramebufferScale.x,
                       imgui_io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(renderer, clear_color.r, clear_color.g,
                           clear_color.b, clear_color.a);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
  }

  auto size = xpano::utils::sdl::GetSize(window);
  xpano::utils::config::Save(app_data_path, size, gui.GetOptions());

  // Cleanup
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  NFD_Quit();
  SDL_Quit();

  return 0;
}
