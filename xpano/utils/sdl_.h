#pragma once

#include <memory>

#include <SDL.h>

namespace xpano::utils::sdl {

struct TexDeleter {
  void operator()(SDL_Texture* tex) { SDL_DestroyTexture(tex); }
};

using Texture = std::unique_ptr<SDL_Texture, TexDeleter>;

class DpiHandler {
 public:
  explicit DpiHandler(SDL_Window* window) : window_(window) {}

  bool DpiChanged() {
    if (float dpi_scale = QueryDpiScale(); dpi_scale != dpi_scale_) {
      dpi_scale_ = dpi_scale;
      return true;
    }
    return false;
  }

  [[nodiscard]] float DpiScale() const { return dpi_scale_; }

 private:
  [[nodiscard]] float QueryDpiScale() const {
    float dpi;
    SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(window_), &dpi, nullptr,
                      nullptr);
    return dpi / 96.0f;
  }
  SDL_Window* window_;
  float dpi_scale_ = 0.0f;
};

}  // namespace xpano::utils::sdl
