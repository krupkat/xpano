// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/utils/sdl_.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include "xpano/constants.h"

namespace xpano::utils::sdl {

std::optional<std::filesystem::path> InitializePrefPath() {
  const std::string org_name{kOrgName};
  const std::string app_name{kAppName};
  auto sdl_pref_path = std::unique_ptr<char, decltype(&SDL_free)>(
      SDL_GetPrefPath(org_name.c_str(), app_name.c_str()), &SDL_free);
  if (!sdl_pref_path) {
    return {};
  }
  return {sdl_pref_path.get()};
}

std::optional<std::filesystem::path> InitializeBasePath() {
  if (const char* sdl_base_path = SDL_GetBasePath()) {
    return {sdl_base_path};
  }
  return {};
}

WindowSize GetSize(SDL_Window* window) {
  WindowSize size;
  SDL_GetWindowSize(window, &size.width, &size.height);
  return size;
}

}  // namespace xpano::utils::sdl
