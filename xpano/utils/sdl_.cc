// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/utils/sdl_.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <SDL.h>
#include <spdlog/spdlog.h>

#include "xpano/constants.h"

namespace xpano::utils::sdl {

WindowManager DetermineWindowManager(bool wayland_supported) {
#ifdef _WIN32
  spdlog::info("WM: Windows");
  return WindowManager::kWindows;
#endif
#ifdef __APPLE__
  spdlog::info("WM: MacOS");
  return WindowManager::kMacOS;
#endif
  const char* video_driver = SDL_GetCurrentVideoDriver();
  if (std::strcmp(video_driver, "wayland") == 0) {
    spdlog::info("WM: Wayland");
    return WindowManager::kWayland;
  }
  if (std::strcmp(video_driver, "x11") == 0) {
    if (wayland_supported) {
      spdlog::info("WM: XWayland");
      spdlog::warn(
          "XWayland doesn't support sharp fractional scaling\nSwitch to "
          "Wayland by running \"export "
          "SDL_VIDEODRIVER=wayland\"");
      return WindowManager::kXWayland;
    }
    spdlog::info("WM: x11");
    return WindowManager::kX11;
  }
  spdlog::info("WM: GenericLinux: {}", video_driver);
  return WindowManager::kGenericLinux;
}

struct RendererFlagInfo {
  SDL_RendererFlags flag;
  std::string label;
};

void PrintRenderDrivers() {
  const int num_drivers = SDL_GetNumRenderDrivers();
  for (int i = 0; i < num_drivers; i++) {
    SDL_RendererInfo info;
    SDL_GetRenderDriverInfo(i, &info);
    spdlog::info("{}: {}", i, info.name);
    const std::vector<RendererFlagInfo> caps = {
        {SDL_RENDERER_SOFTWARE, "the renderer is a software fallback"},
        {SDL_RENDERER_ACCELERATED, "the renderer uses hardware acceleration"},
        {SDL_RENDERER_PRESENTVSYNC,
         "present is synchronized with the refresh rate"},
        {SDL_RENDERER_TARGETTEXTURE,
         "the renderer supports rendering to texture"}};
    for (const auto& [flag, label] : caps) {
      if ((info.flags & flag) != 0u) {
        spdlog::info("\t{}", label);
      }
    }
  }
}

DpiHandler::DpiHandler(SDL_Window* window, WindowManager window_manager)
    : window_(window), window_manager_(window_manager) {}

bool DpiHandler::DpiChanged() {
  if (const float dpi_scale = QueryDpiScale(); dpi_scale != dpi_scale_) {
    dpi_scale_ = dpi_scale;
    spdlog::info("Loading fonts at {}x scale", dpi_scale);
    return true;
  }
  return false;
}

float DpiHandler::DpiScale() const { return dpi_scale_; }

// NOLINTBEGIN(bugprone-branch-clone): doesn't work with [[fallthrough]]

float DpiHandler::QueryDpiScale() const {
  switch (window_manager_) {
    case WindowManager::kWindows:
      [[fallthrough]];
    case WindowManager::kMacOS: {
      float dpi;
      SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(window_), &dpi, nullptr,
                        nullptr);
      return dpi / 96.0f;
    }
    // Wayland supports sharp fractional scaling, use framebuffer scale
    // instead of unreliable https://wiki.libsdl.org/SDL_GetDisplayDPI
    case WindowManager::kWayland: {
      int width;
      int height;
      int display_width;
      int display_height;
      SDL_GetWindowSize(window_, &width, &height);
      SDL_GL_GetDrawableSize(window_, &display_width, &display_height);
      return static_cast<float>(display_width) / static_cast<float>(width);
    }
    case WindowManager::kGenericLinux:
      [[fallthrough]];
    case WindowManager::kXWayland:
      [[fallthrough]];
    case WindowManager::kX11: {
#if SDL_VERSION_ATLEAST(2, 24, 0)
      // https://github.com/libsdl-org/SDL/issues/5764 changed the GetDisplayDPI
      // logic, this is a hack to get consistent scaling factor. This code
      // shouldn't be triggered often as we prefer to run in Wayland.
      float max_dpi = 96.0f;
      int num_displays = SDL_GetNumVideoDisplays();
      for (int i = 0; i < num_displays; i++) {
        float dpi;
        SDL_GetDisplayDPI(i, &dpi, nullptr, nullptr);
        max_dpi = std::max(max_dpi, dpi);
      }
      return max_dpi / 96.0f;
#else
      float hdpi;  // Gets the result of X11_XGetDefault(disp, "Xft", "dpi"),
                   // which is a multiple of 96
      SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(window_), nullptr, &hdpi,
                        nullptr);
      return hdpi / 96.0f;
#endif
    }
    default:
      return 1.0f;
  }
}

// NOLINTEND(bugprone-branch-clone)

std::optional<std::filesystem::path> InitializePrefPath() {
  auto sdl_pref_path = std::unique_ptr<char, decltype(&SDL_free)>(
      SDL_GetPrefPath(kOrgName.c_str(), kAppName.c_str()), &SDL_free);
  if (!sdl_pref_path) {
    return {};
  }
  return {sdl_pref_path.get()};
}

std::optional<std::filesystem::path> InitializeBasePath() {
  auto sdl_base_path =
      std::unique_ptr<char, decltype(&SDL_free)>(SDL_GetBasePath(), &SDL_free);
  if (!sdl_base_path) {
    return {};
  }
  return {sdl_base_path.get()};
}

WindowSize GetSize(SDL_Window* window) {
  WindowSize size;
  SDL_GetWindowSize(window, &size.width, &size.height);
  return size;
}

}  // namespace xpano::utils::sdl
