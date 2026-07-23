// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include <opencv2/stitching.hpp>

namespace xpano::utils::opencv {

std::vector<cv::detail::CameraParams> Scale(
    const std::vector<cv::detail::CameraParams>& cameras, double scale);

cv::Mat ToFloat(const cv::Mat& image);

float MPx(const cv::Rect& rect);

float MPx(const cv::Mat& image);

}  // namespace xpano::utils::opencv
