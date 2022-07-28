#pragma once

#include <opencv2/core.hpp>
#include <SDL.h>

#include "utils/sdl_.h"
#include "utils/vec.h"

namespace xpano::gui {

class PreviewPane {
 public:
  explicit PreviewPane(SDL_Renderer *renderer);
  void Load(cv::Mat image);
  void Draw();

 private:
  utils::sdl::Texture tex_;
  utils::Ratio2f tex_coord_;

  SDL_Renderer *renderer_;
};

}  // namespace xpano::gui
