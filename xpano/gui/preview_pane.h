#pragma once

#include <opencv2/core.hpp>
#include <SDL.h>

#include "gui/action.h"
#include "gui/coord.h"
#include "utils/sdl_.h"

namespace xpano::gui {

class PreviewPane {
 public:
  explicit PreviewPane(SDL_Renderer *renderer);
  void Load(cv::Mat image);
  Action Draw();

 private:
  utils::sdl::Texture tex_;
  Coord coord_;

  float zoom_ = 1.0f;
  ImVec2 image_offset_;
  ImVec2 screen_offset_;

  SDL_Renderer *renderer_;
};

}  // namespace xpano::gui
