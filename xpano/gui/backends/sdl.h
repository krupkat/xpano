#pragma once

#include <imgui.h>
#include <opencv2/core.hpp>
#include <SDL.h>

#include "xpano/gui/backends/base.h"
#include "xpano/utils/vec.h"

namespace xpano::gui::backends {

class Sdl final : public Base {
 public:
  explicit Sdl(SDL_Renderer* renderer);

  Texture CreateTexture(utils::Vec2i size) override;
  void UpdateTexture(ImTextureID tex, cv::Mat image) override;
  void DestroyTexture(ImTextureID tex) override;

 private:
  SDL_Renderer* renderer_;
  SDL_RendererInfo info_;
};

}  // namespace xpano::gui::backends
