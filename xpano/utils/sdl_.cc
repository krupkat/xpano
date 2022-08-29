#include "utils/sdl_.h"

#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>

#include <SDL.h>
#include <spdlog/spdlog.h>

#include "constants.h"

namespace xpano::utils::sdl {

DpiHandler::DpiHandler(SDL_Window* window) : window_(window) {
  const char* video_driver = SDL_GetCurrentVideoDriver();
  linux_ = (std::strcmp(video_driver, "wayland") == 0) ||
           (std::strcmp(video_driver, "x11") == 0);
}

bool DpiHandler::DpiChanged() {
  if (float dpi_scale = QueryDpiScale(); dpi_scale != dpi_scale_) {
    dpi_scale_ = dpi_scale;
    spdlog::info("Loading at {}", dpi_scale);
    return true;
  }
  return false;
}

float DpiHandler::DpiScale() const { return dpi_scale_; }

float DpiHandler::QueryDpiScale() const {
  if (linux_) {
    int width, height;
    int display_width, display_height;
    SDL_GetWindowSize(window_, &width, &height);
    SDL_GL_GetDrawableSize(window_, &display_width, &display_height);
    return static_cast<float>(display_width) / static_cast<float>(width);
  } else {
    float dpi;
    SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(window_), &dpi, nullptr,
                      nullptr);
    return dpi / 96.0f;
  }
}

std::optional<std::filesystem::path> InitializePrefPath() {
  auto sdl_pref_path = std::unique_ptr<char, decltype(&SDL_free)>(
      SDL_GetPrefPath(kOrgName.c_str(), kAppName.c_str()), &SDL_free);
  if (!sdl_pref_path) {
    return {};
  }
  return {sdl_pref_path.get()};
}

}  // namespace xpano::utils::sdl
