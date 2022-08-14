#include <clocale>
#include <cstdio>
#include <string>

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
#include "utils/sdl_.h"

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

int main(int /*unused*/, char** /*unused*/) {
#if SDL_VERSION_ATLEAST(2, 23, 1)
  SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif

  // This feature isn't compatible with ImGui as of v1.88
  // SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    printf("Error: %s\n", SDL_GetError());
    return -1;
  }

  // Setup logging
  xpano::logger::LoggerGui logger{};
  logger.RedirectSpdlogOutput();
  logger.RedirectSDLOutput();

  std::string result = std::setlocale(LC_ALL, "en_US.UTF-8");
  spdlog::info("Current locale: {}", result);

  if (NFD_Init() != NFD_OKAY) {
    spdlog::error("Couldn't initialize NFD");
  }

  // Setup window
  auto window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
  SDL_Window* window =
      SDL_CreateWindow("Dear ImGui SDL2+SDL_Renderer example",
                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       xpano::kWindowWidth, xpano::kWindowHeight, window_flags);

  // Setup SDL_Renderer instance
  SDL_Renderer* renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
  if (renderer == nullptr) {
    spdlog::error("Error creating SDL_Renderer!");
    return -1;
  }
  xpano::gui::backends::Sdl backend{renderer};

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  // ImGui::StyleColorsClassic();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer_Init(renderer);

  ImGuiIO& imgui_io = ImGui::GetIO();
  imgui_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  // Our state
  const SDL_Color clear_color{114, 140, 165, 255};
  xpano::gui::PanoGui gui(&backend, &logger);

  xpano::utils::sdl::DpiHandler dpi_handler(window);
  xpano::utils::imgui::FontLoader font_loader(
      "fonts/NotoSans-Regular.ttf", "fonts/NotoSansSymbols2-Regular.ttf");
  if (!font_loader.Init()) {
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
