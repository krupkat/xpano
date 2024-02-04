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
#include "xpano/gui/action.h"
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

enum class EdgeType : int {
  kTop = 1,
  kBottom = 2,
  kLeft = 4,
  kRight = 8,
  kHorizontal = 16,
  kVertical = 32,
  kRoll = 64
};

struct Edge {
  EdgeType type;
  bool dragging = false;
  bool mouse_close = false;
};

constexpr auto DefaultEdges() {
  return std::array{Edge{EdgeType::kTop}, Edge{EdgeType::kBottom},
                    Edge{EdgeType::kLeft}, Edge{EdgeType::kRight}};
}

enum class CropMode { kInitial, kEnabled, kDisabled };

enum class RotateMode { kEnabled, kDisabled };

struct DraggableWidget {
  utils::RectRRf rect = utils::DefaultCropRect();
  std::array<Edge, 4> edges = DefaultEdges();
};

struct Projectable {
  int camera_id;
  std::vector<cv::Point2f> points;
  cv::Point2f translation;
};

using Polyline = std::vector<ImVec2>;

struct PreprocessedCamera {
  cv::Mat k_mat;  // in CV_32F as expected by warper functions
  cv::Mat r_mat;
};

constexpr auto DefaultEdgesRotation() {
  return std::array{Edge{EdgeType::kHorizontal}, Edge{EdgeType::kVertical},
                    Edge{EdgeType::kRoll}};
}

struct RotationState {
  float yaw = 0.0f;
  float pitch = 0.0f;
  float roll = 0.0f;

  float yaw_start = 0.0f;
  float pitch_start = 0.0f;
  float roll_start = 0.0f;

  utils::Point2f mouse_start;

  std::array<Edge, 3> edges = DefaultEdgesRotation();
};

struct StaticWarpData {
  cv::Size scale;
  std::vector<PreprocessedCamera> cameras;
  cv::Ptr<cv::detail::RotationWarper> warper;
  cv::Mat rollAxis;
  cv::Mat pitchAxis;
  cv::Mat yawAxis;
};

struct RotationWidget {
  Projectable horizontal_handle;
  Projectable vertical_handle;
  std::vector<Projectable> image_borders;

  StaticWarpData warp;
  RotationState rotation;
};

class PreviewPane {
 public:
  explicit PreviewPane(backends::Base* backend);
  void Load(cv::Mat image, ImageType image_type);
  void Reload(cv::Mat image, ImageType image_type);
  Action Draw(const std::string& message);
  void Reset();
  Action ToggleCrop();
  Action ToggleRotate();
  bool IsRotateEnabled() const;
  void EndCrop();
  void EndRotate();
  void ForceCrop(const utils::RectRRf& rect);
  void SetSuggestedCrop(const utils::RectRRf& rect);
  void SetCameras(const algorithm::Cameras& cameras);

  [[nodiscard]] ImageType Type() const;
  [[nodiscard]] cv::Mat Image() const;

 private:
  [[nodiscard]] float Zoom() const;
  [[nodiscard]] bool IsZoomed() const;
  void ZoomIn();
  void ZoomOut();
  void AdvanceZoom();
  void ResetZoom(int target_level = 1);
  Action HandleInputs(const utils::RectPVf& window, const utils::RectPVf& image);

  utils::Ratio2f tex_coord_;

  CropMode crop_mode_ = CropMode::kInitial;
  RotateMode rotate_mode_ = RotateMode::kDisabled;
  DraggableWidget crop_widget_;
  RotationWidget rotate_widget_;
  utils::RectRRf suggested_crop_ = utils::DefaultCropRect();
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
