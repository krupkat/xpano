// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>

#include <SDL3/SDL.h>

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

std::optional<std::filesystem::path> InitializeBasePath();

struct WindowSize {
  int width;
  int height;
};

WindowSize GetSize(SDL_Window* window);

}  // namespace xpano::utils::sdl
