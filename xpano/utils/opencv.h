// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include <opencv2/core/version.hpp>
#include <opencv2/stitching.hpp>

#define XPANO_OPENCV_HAS_JPEG_SUBSAMPLING_SUPPORT \
  (CV_VERSION_MAJOR >= 4 && CV_VERSION_MINOR >= 7)

#define XPANO_OPENCV_HAS_NEW_DRAW_MATCHES_API \
  (CV_VERSION_MAJOR >= 4 && CV_VERSION_MINOR >= 5 && CV_VERSION_REVISION >= 3)

namespace xpano::utils::opencv {

constexpr bool HasJpegSubsamplingSupport() {
  return XPANO_OPENCV_HAS_JPEG_SUBSAMPLING_SUPPORT;
}

std::vector<cv::detail::CameraParams> Scale(
    const std::vector<cv::detail::CameraParams> &cameras, double scale);

cv::Mat ToFloat(const cv::Mat &image);

float MPx(const cv::Rect &rect);

float MPx(const cv::Mat &image);

}  // namespace xpano::utils::opencv
