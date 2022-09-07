#pragma once

#include <array>
#include <string>

#include <opencv2/core.hpp>

#include "xpano/constants.h"
#include "xpano/gui/backends/base.h"
#include "xpano/utils/vec.h"

namespace xpano::gui {

class PreviewPane {
 public:
  explicit PreviewPane(backends::Base* backend);
  void Load(cv::Mat image);
  void Draw(const std::string& message);
  void Reset();

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
