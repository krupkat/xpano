// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/stitching/detail/motion_estimators.hpp>

namespace xpano::algorithm {

class BundleAdjusterRayCustom : public cv::detail::BundleAdjusterRay {
 public:
  [[nodiscard]] cv::detail::WaveCorrectKind WaveCorrectionKind() const;

 private:
  void setUpInitialCameraParams(
      const std::vector<cv::detail::CameraParams>& cameras) override;

  cv::detail::WaveCorrectKind kind_;
};

}  // namespace xpano::algorithm
