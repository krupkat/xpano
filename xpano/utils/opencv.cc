// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/utils/opencv.h"

#include <algorithm>
#include <vector>

#include <opencv2/stitching.hpp>

namespace xpano::utils::opencv {

std::vector<cv::detail::CameraParams> Scale(
    const std::vector<cv::detail::CameraParams> &cameras, double scale) {
  std::vector<cv::detail::CameraParams> scaled_cameras;
  std::transform(cameras.begin(), cameras.end(),
                 std::back_inserter(scaled_cameras),
                 [scale](const auto &camera) {
                   cv::detail::CameraParams scaled_camera = camera;
                   scaled_camera.focal *= scale;
                   scaled_camera.ppx *= scale;
                   scaled_camera.ppy *= scale;
                   return scaled_camera;
                 });
  return scaled_cameras;
}

cv::Mat ToFloat(const cv::Mat &image) {
  cv::Mat float_image;
  image.convertTo(float_image, CV_32F);
  return float_image;
}

}  // namespace xpano::utils::opencv