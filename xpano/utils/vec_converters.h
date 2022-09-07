#pragma once

#include <imgui.h>
#include <opencv2/core.hpp>
#include <SDL.h>

#include "xpano/utils/vec.h"

namespace xpano::utils {

constexpr Vec2f ToVec(const ImVec2& imvec) { return Vec2f{imvec.x, imvec.y}; }
constexpr Vec2i ToIntVec(const ImVec2& imvec) {
  return Vec2i{static_cast<int>(imvec.x), static_cast<int>(imvec.y)};
}
constexpr Point2f ToPoint(const ImVec2& imvec) {
  return Point2f{imvec.x, imvec.y};
}
constexpr Point2i ToIntPoint(const ImVec2& imvec) {
  return Point2i{static_cast<int>(imvec.x), static_cast<int>(imvec.y)};
}
template <typename TType, typename NameTag>
constexpr ImVec2 ImVec(const Vec<TType, 2, NameTag>& vec) {
  return {static_cast<float>(vec[0]), static_cast<float>(vec[1])};
}
inline cv::Rect CvRect(Point2i start, Vec2i size) {
  return cv::Rect{start[0], start[1], size[0], size[1]};
}
inline cv::Size CvSize(Vec2i size) { return cv::Size{size[0], size[1]}; }
inline Vec2i ToIntVec(const cv::MatSize& size) {
  return Vec2i{size[1], size[0]};
}
inline SDL_Rect SdlRect(Point2i start, Vec2i size) {
  return SDL_Rect{start[0], start[1], size[0], size[1]};
}

}  // namespace xpano::utils
