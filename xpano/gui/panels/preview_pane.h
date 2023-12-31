// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <optional>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/stitching/detail/warpers.hpp>

#include "xpano/algorithm/algorithm.h"
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

enum class RotateMode { kEnabled, kDisabled };

struct DraggableWidget {
  utils::RectRRf rect = DefaultCropRect();
  std::array<Edge, 4> edges = DefaultEdges();
};

struct Projectable {
  int camera_id;
  std::vector<cv::Point2f> points;
  cv::Point2f translation;
};

struct PreprocessedCamera {
  cv::Mat k_mat;  // in CV_32F as expected by warper functions
  cv::Mat r_mat;
};

struct RotationWidget {
  Projectable horizontal_handle;
  Projectable vertical_handle;
  Projectable roll_handle;
  std::vector<Projectable> image_borders;

  cv::Size scale;
  std::vector<PreprocessedCamera> cameras;
  cv::Ptr<cv::detail::RotationWarper> warper;

  float yaw = 0.0f;
  float pitch = 0.0f;
  float roll = 0.0f;
};

class PreviewPane {
 public:
  explicit PreviewPane(backends::Base* backend);
  void Load(cv::Mat image, ImageType image_type);
  void Reload(cv::Mat image, ImageType image_type);
  void Draw(const std::string& message);
  void Reset();
  void ToggleCrop();
  void ToggleRotate();
  void EndCrop();
  void EndRotate();
  void SetSuggestedCrop(const utils::RectRRf& rect);
  void SetCameras(const algorithm::Cameras& cameras);

  [[nodiscard]] ImageType Type() const;
  [[nodiscard]] cv::Mat Image() const;
  [[nodiscard]] utils::RectRRf CropRect() const;

 private:
  [[nodiscard]] float Zoom() const;
  [[nodiscard]] bool IsZoomed() const;
  void ZoomIn();
  void ZoomOut();
  void AdvanceZoom();
  void ResetZoom(int target_level = 1);
  void HandleInputs(const utils::RectPVf& window, const utils::RectPVf& image);

  utils::Ratio2f tex_coord_;

  CropMode crop_mode_ = CropMode::kInitial;
  RotateMode rotate_mode_ = RotateMode::kDisabled;
  DraggableWidget crop_widget_;
  RotationWidget rotate_widget_;
  utils::RectRRf suggested_crop_ = DefaultCropRect();
  std::optional<algorithm::Cameras> cameras_;

  int zoom_id_ = 1;
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
