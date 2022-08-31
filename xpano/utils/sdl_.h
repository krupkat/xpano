#pragma once

#include <filesystem>
#include <optional>

#include <SDL.h>

namespace xpano::utils::sdl {

enum class WindowManager {
  kWindows,
  kMacOS,
  kX11,
  kWayland,
  kXWayland,
  kGenericLinux
};

WindowManager DetermineWindowManager(bool wayland_supported);

class DpiHandler {
 public:
  explicit DpiHandler(SDL_Window* window, WindowManager window_manager);

  bool DpiChanged();
  [[nodiscard]] float DpiScale() const;

 private:
  [[nodiscard]] float QueryDpiScale() const;

  SDL_Window* window_;
  WindowManager window_manager_;
  float dpi_scale_ = 0.0f;
};

std::optional<std::filesystem::path> InitializePrefPath();

std::optional<std::filesystem::path> InitializeBasePath();

}  // namespace xpano::utils::sdl
