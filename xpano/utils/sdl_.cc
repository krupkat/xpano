#include "utils/sdl_.h"

#include <filesystem>
#include <memory>
#include <optional>

#include <SDL.h>

#include "constants.h"

namespace xpano::utils::sdl {

DpiHandler::DpiHandler(SDL_Window* window) : window_(window) {}

bool DpiHandler::DpiChanged() {
  if (float dpi_scale = QueryDpiScale(); dpi_scale != dpi_scale_) {
    dpi_scale_ = dpi_scale;
    return true;
  }
  return false;
}

float DpiHandler::DpiScale() const { return dpi_scale_; }

float DpiHandler::QueryDpiScale() const {
  float dpi;
  SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(window_), &dpi, nullptr, nullptr);
  return dpi / 96.0f;
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
