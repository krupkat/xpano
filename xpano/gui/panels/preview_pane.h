#pragma once

#include <array>
#include <string>

#include <opencv2/core.hpp>

#include "xpano/constants.h"
#include "xpano/gui/backends/base.h"
#include "xpano/utils/vec.h"

namespace xpano::gui {

enum class ImageType {
  kNone,
  kSingleImage,
  kMatch,
  kPanoPreview,
  kPanoFullRes
};

class PreviewPane {
 public:
  explicit PreviewPane(backends::Base* backend);
  void Load(cv::Mat image, ImageType image_type);
  void Draw(const std::string& message);
  void Reset();

  [[nodiscard]] ImageType Type() const;

 private:
  [[nodiscard]] float Zoom() const;
  [[nodiscard]] bool IsZoomed() const;
  void ZoomIn();
  void ZoomOut();
  void AdvanceZoom();
  void ResetZoom();

  utils::Ratio2f tex_coord_;

  int zoom_id_ = 0;
  float zoom_ = 1.0f;
  std::array<float, kZoomLevels> zoom_levels_;

  utils::Ratio2f image_offset_;
  utils::Ratio2f screen_offset_;

  backends::Texture tex_;
  backends::Base* backend_;

  ImageType image_type_ = ImageType::kNone;
  cv::Mat full_resolution_pano_;
};

}  // namespace xpano::gui
