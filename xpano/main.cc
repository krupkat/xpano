/*
    Xpano - a tool for stitching photos into panoramas.
    Copyright (C) 2022  Tomas Krupka

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <clocale>
#include <cstdio>
#include <string>
#include <utility>

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_sdlrenderer.h>
#include <nfd.h>
#include <SDL.h>
#include <spdlog/spdlog.h>

#include "constants.h"
#include "gui/backends/sdl.h"
#include "gui/pano_gui.h"
#include "log/logger.h"
#include "utils/imgui_.h"
#include "utils/resource.h"
#include "utils/sdl_.h"
#include "utils/text.h"

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

int main(int /*unused*/, char** argv) {
#if SDL_VERSION_ATLEAST(2, 23, 1)
  SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif

  // This feature isn't compatible with ImGui as of v1.88
  // SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    printf("Error: %s\n", SDL_GetError());
    return -1;
  }

  auto app_data_path = xpano::utils::sdl::InitializePrefPath();

  // Setup logging
  xpano::logger::LoggerGui logger{};
  logger.RedirectSpdlogOutput(app_data_path);
  logger.RedirectSDLOutput();

  std::string result = std::setlocale(LC_ALL, "en_US.UTF-8");
  spdlog::info("Current locale: {}", result);

  if (!app_data_path) {
    spdlog::warn(
        "Failed to initialize application data path, skipping logging to "
        "files.");
  }

  // Setup file dialog library
  if (NFD_Init() != NFD_OKAY) {
    spdlog::error("Couldn't initialize NFD");
  }

  // Setup SDL Window + Renderer
  auto window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
  SDL_Window* window =
      SDL_CreateWindow("Xpano", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       xpano::kWindowWidth, xpano::kWindowHeight, window_flags);

  SDL_Renderer* renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
  if (renderer == nullptr) {
    spdlog::error("Error creating SDL_Renderer!");
    return -1;
  }

  auto icon = xpano::utils::resource::LoadIcon(argv[0], xpano::kIconPath);
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
  ImGui_ImplSDLRenderer_Init(renderer);

  const SDL_Color clear_color{114, 140, 165, 255};

  // Application specific
  auto backend = xpano::gui::backends::Sdl{renderer};

  std::future<xpano::utils::Texts> license_texts =
      std::async(std::launch::async, xpano::utils::LoadTexts, argv[0],
                 xpano::kLicensePath);

  xpano::gui::PanoGui gui(&backend, &logger, std::move(license_texts));

  xpano::utils::sdl::DpiHandler dpi_handler(window);
  xpano::utils::imgui::FontLoader font_loader(xpano::kFontPath,
                                              xpano::kSymbolsFontPath);
  if (!font_loader.Init(argv[0])) {
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
    ImGui_ImplSDLRenderer_NewFrame();
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
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(renderer);
  }

  // Cleanup
  ImGui_ImplSDLRenderer_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  NFD_Quit();
  SDL_Quit();

  return 0;
}
