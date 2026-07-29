// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include <opencv2/core/version.hpp>
#include <opencv2/stitching.hpp>

#define XPANO_OPENCV_VERSIONNUM(major, minor, patch) \
    ((major) * 1000000 + (minor) * 1000 + (patch))

#define XPANO_OPENCV_VERSION \
    XPANO_OPENCV_VERSIONNUM(CV_VERSION_MAJOR, CV_VERSION_MINOR, CV_VERSION_REVISION)

#define XPANO_OPENCV_VERSION_ATLEAST(X, Y, Z) \
    (XPANO_OPENCV_VERSION >= XPANO_OPENCV_VERSIONNUM(X, Y, Z))

#define XPANO_OPENCV_HAS_JPEG_SUBSAMPLING_SUPPORT \
  XPANO_OPENCV_VERSION_ATLEAST(4, 7, 0)

#define XPANO_OPENCV_HAS_NEW_DRAW_MATCHES_API \
  XPANO_OPENCV_VERSION_ATLEAST(4, 5, 3)

namespace xpano::utils::opencv {

constexpr bool HasJpegSubsamplingSupport() {
  return XPANO_OPENCV_HAS_JPEG_SUBSAMPLING_SUPPORT;
}

std::vector<cv::detail::CameraParams> Scale(
    const std::vector<cv::detail::CameraParams>& cameras, double scale);

cv::Mat ToFloat(const cv::Mat& image);

float MPx(const cv::Rect& rect);

float MPx(const cv::Mat& image);

}  // namespace xpano::utils::opencv
