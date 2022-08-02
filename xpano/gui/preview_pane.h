#pragma once

#include <array>

#include <opencv2/core.hpp>
#include <SDL.h>

#include "constants.h"
#include "utils/sdl_.h"
#include "utils/vec.h"

namespace xpano::gui {

class PreviewPane {
 public:
  explicit PreviewPane(SDL_Renderer *renderer);
  void Load(cv::Mat image);
  void Draw();

 private:
  [[nodiscard]] float Zoom() const;
  [[nodiscard]] bool IsZoomed() const;
  void ZoomIn();
  void ZoomOut();

  utils::sdl::Texture tex_;
  utils::Ratio2f tex_coord_;

  int zoom_id_ = 0;
  std::array<float, kZoomLevels> zoom_;

  utils::Ratio2f image_offset_;
  utils::Ratio2f screen_offset_;

  SDL_Renderer *renderer_;
};

}  // namespace xpano::gui
