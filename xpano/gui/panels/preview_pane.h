// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <optional>
#include <string>

#include <opencv2/core.hpp>

#include "xpano/algorithm/algorithm.h"
#include "xpano/constants.h"
#include "xpano/gui/action.h"
#include "xpano/gui/backends/base.h"
#include "xpano/gui/widgets/widgets.h"
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

enum class CropMode { kInitial, kEnabled, kDisabled };

enum class RotateMode { kEnabled, kDisabled };

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
  void ResetCrop(const utils::RectRRf& rect);
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
  widgets::DraggableWidget crop_widget_;
  widgets::RotationWidget rotate_widget_;
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
