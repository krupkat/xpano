// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <opencv2/core.hpp>

#include "xpano/utils/vec.h"

namespace xpano::utils {

inline cv::Rect CvRect(Point2i start, Vec2i size) {
  return cv::Rect{start[0], start[1], size[0], size[1]};
}
inline cv::Size CvSize(Vec2i size) { return cv::Size{size[0], size[1]}; }
inline Vec2i ToIntVec(const cv::MatSize& size) {
  return Vec2i{size[1], size[0]};
}

inline cv::Rect GetCvRect(const cv::Mat& image, utils::RectRRf crop_rect) {
  auto image_size = utils::ToIntVec(image.size);
  auto crop_start = utils::Point2f{0.0f} + image_size * crop_rect.start;
  auto crop_size = image_size * (crop_rect.end - crop_rect.start);
  return utils::CvRect(utils::ToIntVec(crop_start), utils::ToIntVec(crop_size));
}

}  // namespace xpano::utils
