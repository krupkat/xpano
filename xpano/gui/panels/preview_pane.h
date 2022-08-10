#pragma once

#include <array>

#include <opencv2/core.hpp>

#include "constants.h"
#include "gui/backends/base.h"
#include "utils/vec.h"

namespace xpano::gui {

class PreviewPane {
 public:
  explicit PreviewPane(backends::Base* backend);
  void Load(cv::Mat image);
  void Draw();

 private:
  [[nodiscard]] float Zoom() const;
  [[nodiscard]] bool IsZoomed() const;
  void ZoomIn();
  void ZoomOut();
  void ResetZoom();

  utils::Ratio2f tex_coord_;

  int zoom_id_ = 0;
  std::array<float, kZoomLevels> zoom_;

  utils::Ratio2f image_offset_;
  utils::Ratio2f screen_offset_;

  backends::Texture tex_;
  backends::Base* backend_;
};

}  // namespace xpano::gui
