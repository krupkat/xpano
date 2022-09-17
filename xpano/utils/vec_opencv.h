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

}  // namespace xpano::utils
