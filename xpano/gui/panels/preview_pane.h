#pragma once

#include <array>
#include <string>

#include <opencv2/core.hpp>

#include "xpano/constants.h"
#include "xpano/gui/backends/base.h"
#include "xpano/utils/rect.h"
#include "xpano/utils/vec.h"

namespace xpano::gui {

enum class ImageType {
  kNone,
  kSingleImage,
  kMatch,
  kPanoPreview,
  kPanoFullRes
};

enum class EdgeType { kTop = 1, kBottom = 2, kLeft = 4, kRight = 8 };

struct Edge {
  EdgeType type;
  bool dragging = false;
  bool mouse_close = false;
};

constexpr auto DefaultEdges() {
  return std::array{Edge{EdgeType::kTop}, Edge{EdgeType::kBottom},
                    Edge{EdgeType::kLeft}, Edge{EdgeType::kRight}};
}

constexpr auto DefaultCropRect() {
  return utils::Rect(utils::Ratio2f{0.0f}, utils::Ratio2f{1.0f});
}

enum class CropMode { kInitial, kEnabled, kDisabled };

struct DraggableWidget {
  utils::RectRRf rect = DefaultCropRect();
  std::array<Edge, 4> edges = DefaultEdges();
};

class PreviewPane {
 public:
  explicit PreviewPane(backends::Base* backend);
  void Load(cv::Mat image, ImageType image_type);
  void Draw(const std::string& message);
  void Reset();
  void ToggleCrop();
  void EndCrop();
  void SetSuggestedCrop(const utils::RectRRf& rect);

  [[nodiscard]] ImageType Type() const;
  [[nodiscard]] cv::Mat Image() const;
  [[nodiscard]] utils::Ratio2f CropStart() const;
  [[nodiscard]] utils::Ratio2f CropEnd() const;

 private:
  [[nodiscard]] float Zoom() const;
  [[nodiscard]] bool IsZoomed() const;
  void ZoomIn();
  void ZoomOut();
  void AdvanceZoom();
  void ResetZoom();
  void HandleInputs(const utils::Point2f& window_start,
                    const utils::Vec2f& window_size,
                    const utils::Point2f& image_start,
                    const utils::Vec2f& image_size);

  utils::Ratio2f tex_coord_;

  CropMode crop_mode_ = CropMode::kInitial;
  DraggableWidget crop_widget_;
  utils::RectRRf suggested_crop_ = DefaultCropRect();

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
