// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <imgui.h>
#include <opencv2/core.hpp>
#include <SDL3/SDL.h>

#include "xpano/gui/backends/base.h"
#include "xpano/utils/vec.h"

namespace xpano::gui::backends {

class Sdl final : public Base {
 public:
  explicit Sdl(SDL_Renderer* renderer);

  Texture CreateTexture(utils::Vec2i size) override;
  void UpdateTexture(ImTextureID tex, cv::Mat image) override;
  void DestroyTexture(ImTextureID tex) noexcept override;

 private:
  SDL_Renderer* renderer_;
  int max_texture_size_;
};

}  // namespace xpano::gui::backends
