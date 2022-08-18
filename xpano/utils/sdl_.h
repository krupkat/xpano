#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include <SDL.h>

namespace xpano::utils::sdl {

class DpiHandler {
 public:
  explicit DpiHandler(SDL_Window* window);

  bool DpiChanged();
  [[nodiscard]] float DpiScale() const;

 private:
  [[nodiscard]] float QueryDpiScale() const;

  SDL_Window* window_;
  float dpi_scale_ = 0.0f;
};

std::optional<std::filesystem::path> InitializePrefPath();

}  // namespace xpano::utils::sdl
