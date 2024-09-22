// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/widgets/rotate.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>

#include <imgui.h>
#include <opencv2/calib3d.hpp>
#include <spdlog/spdlog.h>

#include "xpano/algorithm/algorithm.h"
#include "xpano/constants.h"
#include "xpano/gui/widgets/widgets.h"
#include "xpano/utils/opencv.h"
#include "xpano/utils/rect.h"
#include "xpano/utils/vec.h"
#include "xpano/utils/vec_converters.h"

namespace xpano::gui::widgets {

namespace {

constexpr int kPointsPerEdge = 50;

std::vector<cv::Point2f> PointsOnRectangle(
    cv::Size size, int points_per_edge = kPointsPerEdge) {
  std::vector<cv::Point2f> points;
  points.reserve(points_per_edge * 4 + 1);
  for (int i = 0; i < points_per_edge; i++) {
    points.emplace_back(i, 0);
  }
  for (int i = 0; i < points_per_edge; i++) {
    points.emplace_back(points_per_edge, i);
  }
  for (int i = 0; i < points_per_edge; i++) {
    points.emplace_back(points_per_edge - i, points_per_edge);
  }
  for (int i = 0; i < points_per_edge; i++) {
    points.emplace_back(0, points_per_edge - i);
  }
  points.emplace_back(0, 0);

  std::transform(points.begin(), points.end(), points.begin(),
                 [&size, points_per_edge](const cv::Point& point) {
                   auto rescaled =
                       cv::Point{(point.x * size.width) / points_per_edge,
                                 (point.y * size.height) / points_per_edge};
                   return cv::Point2f{rescaled};
                 });
  return points;
}

std::vector<PreprocessedCamera> Preprocess(
    const std::vector<cv::detail::CameraParams>& cameras, double work_scale) {
  auto scaled_cameras = utils::opencv::Scale(cameras, 1.0 / work_scale);

  std::vector<PreprocessedCamera> result(scaled_cameras.size());
  std::transform(scaled_cameras.begin(), scaled_cameras.end(), result.begin(),
                 [](const cv::detail::CameraParams& camera_params) {
                   return PreprocessedCamera{
                       .k_mat = utils::opencv::ToFloat(camera_params.K()),
                       .r_mat = camera_params.R};
                 });
  return result;
}

template <typename TPoint>
TPoint Avg(const std::vector<TPoint>& points) {
  TPoint sum = std::accumulate(points.begin(), points.end(), TPoint{0.0f, 0.0f},
                               [](const TPoint& lhs, const TPoint& rhs) {
                                 return TPoint{lhs.x + rhs.x, lhs.y + rhs.y};
                               });
  return TPoint{sum.x / points.size(), sum.y / points.size()};
}

struct PanoCenter {
  int id;
  cv::Point2f coords;
};

PanoCenter ComputePanoCenter(const std::vector<cv::Size>& image_sizes,
                             const StaticWarpData& warp) {
  std::vector<cv::Point2f> centers;
  for (int i = 0; i < image_sizes.size(); i++) {
    auto center = cv::Point2f{static_cast<float>(image_sizes[i].width) / 2.0f,
                              static_cast<float>(image_sizes[i].height) / 2.0f};

    auto projected_center = warp.warper->warpPoint(
        center, warp.cameras[i].k_mat, warp.cameras[i].r_mat);
    centers.push_back(projected_center);
  }

  auto dist = [](const cv::Point2f& lhs, const cv::Point2f& rhs) {
    return cv::norm(lhs - rhs);
  };

  auto center = Avg(centers);
  auto middle_image = std::min_element(
      centers.begin(), centers.end(),
      [&center, &dist](const cv::Point2f& left, const cv::Point2f& right) {
        return dist(left, center) < dist(right, center);
      });
  return {static_cast<int>(std::distance(centers.begin(), middle_image)),
          cv::Point2f{center.x, center.y}};
}

std::vector<cv::Point2f> Interpolate(const cv::Point2f& start,
                                     const cv::Point2f& end,
                                     int num_edges = kPointsPerEdge) {
  std::vector<cv::Point2f> points;
  points.reserve(num_edges + 1);
  for (int i = 0; i <= num_edges; i++) {
    const float alpha = static_cast<float>(i) / static_cast<float>(num_edges);
    points.push_back(start + alpha * (end - start));
  }
  return points;
}

Projectable GenericHandle(const cv::Rect& dst_roi, const cv::Point2f& dir,
                          const PanoCenter& center,
                          const StaticWarpData& warp) {
  const auto& camera = warp.cameras[center.id];
  auto backprojected =
      warp.warper->warpPointBackward(center.coords, camera.k_mat, camera.r_mat);

  auto start = backprojected - dir;
  auto end = backprojected + dir;

  return {.camera_id = center.id,
          .points = Interpolate(start, end),
          .translation = -dst_roi.tl()};
}

Projectable HorizontalHandle(const cv::Rect& dst_roi, const PanoCenter& center,
                             const StaticWarpData& warp) {
  auto diff = cv::Point2f{static_cast<float>(dst_roi.width), 0.0f};
  return GenericHandle(dst_roi, diff, center, warp);
}

Projectable VerticalHandle(const cv::Rect& dst_roi, const PanoCenter& center,
                           const StaticWarpData& warp) {
  auto diff = cv::Point2f{0.0f, static_cast<float>(dst_roi.height)};
  return GenericHandle(dst_roi, diff, center, warp);
}

Projectable RollHandle(const cv::Rect& dst_roi, const PanoCenter& center,
                       const StaticWarpData& warp) {
  const auto& camera = warp.cameras[center.id];
  auto backprojected =
      warp.warper->warpPointBackward(center.coords, camera.k_mat, camera.r_mat);

  return {.camera_id = center.id,
          .points = {backprojected},
          .translation = -dst_roi.tl()};
}

cv::Mat Image2Camera(const cv::Point2f& point,
                     const PreprocessedCamera& camera) {
  return camera.r_mat * (camera.k_mat.inv() * cv::Mat{point.x, point.y, 1.0f});
}

template <typename TMat>
TMat Normalized(const TMat& input) {
  return input / cv::norm(input);
}

cv::Mat RollAxis(const PanoCenter& center, const StaticWarpData& warp) {
  const auto& camera = warp.cameras[center.id];
  auto backprojected =
      warp.warper->warpPointBackward(center.coords, camera.k_mat, camera.r_mat);
  auto roll_axis = Image2Camera(backprojected, camera);
  return Normalized(roll_axis);
}

Axis GenericAxis(const PanoCenter& center, const cv::Point2f& dir,
                 const StaticWarpData& warp) {
  const auto& camera = warp.cameras[center.id];
  auto backprojected =
      warp.warper->warpPointBackward(center.coords, camera.k_mat, camera.r_mat);

  auto probe1 = backprojected - dir;
  auto probe2 = backprojected + dir;

  auto p1_camera = Image2Camera(probe1, camera);
  auto p2_camera = Image2Camera(probe2, camera);
  const cv::Mat pitch_axis = p2_camera.cross(p1_camera);
  const double angle = std::asin(cv::norm(pitch_axis) /
                                 (cv::norm(p1_camera) * cv::norm(p2_camera)));

  auto p1_reproj = warp.warper->warpPoint(probe1, camera.k_mat, camera.r_mat);
  auto p2_reproj = warp.warper->warpPoint(probe2, camera.k_mat, camera.r_mat);
  auto diff_reproj = p1_reproj - p2_reproj;
  const double distance = cv::norm(diff_reproj);

  return {Normalized(pitch_axis), Normalized(diff_reproj),
          static_cast<float>(angle / distance)};
}

Axis PitchAxis(const PanoCenter& center, const StaticWarpData& warp) {
  const cv::Point2f diff = {0.0f, 50.0f};
  return GenericAxis(center, diff, warp);
}

Axis YawAxis(const PanoCenter& center, const StaticWarpData& warp) {
  const cv::Point2f diff = {50.0f, 0.0f};
  auto axis = GenericAxis(center, diff, warp);
  axis.coords *= -1.0f;
  return axis;
}

// NOLINTBEGIN(readability-identifier-length)
float PointToSegmentDistance(const ImVec2& a, const ImVec2& b,
                             const ImVec2& p) {
  auto ap = ImVec2{p.x - a.x, p.y - a.y};
  auto ab = ImVec2{b.x - a.x, b.y - a.y};
  auto dot = [](const ImVec2& a, const ImVec2& b) {
    return a.x * b.x + a.y * b.y;
  };
  const float dot_ab = dot(ab, ab);
  if (dot_ab < 1e-6f) {
    return dot(ap, ap);
  }
  auto t = std::clamp(dot(ap, ab) / dot_ab, 0.0f, 1.0f);
  auto projected = ImVec2{a.x + t * ab.x, a.y + t * ab.y};
  auto diff = ImVec2{p.x - projected.x, p.y - projected.y};
  return dot(diff, diff);
}
// NOLINTEND(readability-identifier-length)

bool IsMouseCloseToPoly(const Polyline& poly, utils::Point2f mouse_pos) {
  for (int i = 0; i < poly.size() - 1; i++) {
    auto dist = PointToSegmentDistance(poly[i], poly[i + 1],
                                       ImVec2{mouse_pos[0], mouse_pos[1]});
    if (dist < kCropEdgeTolerance * kCropEdgeTolerance) {
      return true;
    }
  }

  return false;
}

float Project(const cv::Point2f& diff, const cv::Point2f& target_dir) {
  auto diff_length = cv::norm(diff);
  if (diff_length < 1e-6f) {
    return 0.0f;
  }
  auto cos_theta = diff.dot(target_dir) / diff_length;
  return static_cast<float>(diff_length * cos_theta);
}

float ComputePitch(const utils::Vec2f& mouse_diff, const StaticWarpData& warp,
                   const utils::RectPVf& image) {
  const float speed = warp.pitch_axis.rot_speed *
                      (static_cast<float>(warp.scale.width) / image.size[0]);

  return -1.0f * speed *
         Project(utils::CvPoint(mouse_diff), warp.pitch_axis.drag_dir);
}

float ComputeYaw(const utils::Vec2f& mouse_diff, const StaticWarpData& warp,
                 const utils::RectPVf& image) {
  const float speed = warp.yaw_axis.rot_speed *
                      (static_cast<float>(warp.scale.height) / image.size[1]);

  return speed * Project(utils::CvPoint(mouse_diff), warp.yaw_axis.drag_dir);
}

float ComputeRoll(const utils::Point2f& mouse_start,
                  const utils::Point2f& mouse_end,
                  const utils::Point2f& roll_center) {
  auto d_x = mouse_start - roll_center;
  auto d_y = mouse_end - roll_center;

  return std::atan2(d_x[0], d_x[1]) - std::atan2(d_y[0], d_y[1]);
}

bool IsHorizontal(const cv::Point2f& dir) {
  return std::abs(dir.x) > std::abs(dir.y);
}

}  // namespace

RotationWidget SetupRotationWidget(const algorithm::Cameras& cameras) {
  const int num_cameras = static_cast<int>(cameras.cameras.size());

  auto dst_roi = cv::detail::resultRoi(cameras.warp_helper.corners,
                                       cameras.warp_helper.sizes);

  spdlog::debug("ROT: dst_roi = x {}, y {}, width {}, height{}", dst_roi.tl().x,
                dst_roi.tl().y, dst_roi.size().width, dst_roi.size().height);

  std::vector<Projectable> projectables(num_cameras);

  for (int i = 0; i < num_cameras; i++) {
    const auto& size = cameras.warp_helper.full_sizes[i];

    projectables[i].camera_id = i;
    projectables[i].translation = -dst_roi.tl();
    projectables[i].points = PointsOnRectangle(size);

    spdlog::debug("ROT: cam {}, translation x {}, y {}", i,
                  projectables[i].translation.x, projectables[i].translation.y);
  }

  auto preprocessed_cameras =
      Preprocess(cameras.cameras, cameras.warp_helper.work_scale);

  auto warp = StaticWarpData{.scale = dst_roi.size(),
                             .cameras = std::move(preprocessed_cameras),
                             .warper = cameras.warp_helper.warper};

  auto pano_center = ComputePanoCenter(cameras.warp_helper.full_sizes, warp);

  auto vertical_handle = VerticalHandle(dst_roi, pano_center, warp);
  auto horizontal_handle = HorizontalHandle(dst_roi, pano_center, warp);
  auto roll_handle = RollHandle(dst_roi, pano_center, warp);

  warp.roll_axis = RollAxis(pano_center, warp);
  warp.pitch_axis = PitchAxis(pano_center, warp);
  warp.yaw_axis = YawAxis(pano_center, warp);

  return {.horizontal_handle = std::move(horizontal_handle),
          .vertical_handle = std::move(vertical_handle),
          .roll_handle = std::move(roll_handle),
          .image_borders = std::move(projectables),
          .warp = std::move(warp)};
}

cv::Mat FullRotation(const RotationState& state, const StaticWarpData& warp) {
  auto rot = cv::Mat::eye(3, 3, CV_32F);

  if (state.roll != 0.0f) {
    const cv::Mat rotation_vector = state.roll * warp.roll_axis;
    cv::Mat rotation_matrix;
    cv::Rodrigues(rotation_vector, rotation_matrix);
    rot = rotation_matrix * rot;
  }

  if (state.pitch != 0.0f) {
    const cv::Mat rotation_vector = state.pitch * warp.pitch_axis.coords;
    cv::Mat rotation_matrix;
    cv::Rodrigues(rotation_vector, rotation_matrix);
    rot = rotation_matrix * rot;
  }

  if (state.yaw != 0.0f) {
    const cv::Mat rotation_vector = state.yaw * warp.yaw_axis.coords;
    cv::Mat rotation_matrix;
    cv::Rodrigues(rotation_vector, rotation_matrix);
    rot = rotation_matrix * rot;
  }

  return rot;
}

Polyline Warp(const Projectable& projectable, const StaticWarpData& warp,
              const RotationState& state, const utils::RectPVf& image) {
  const int camera_id = projectable.camera_id;
  const auto& camera = warp.cameras[camera_id];
  cv::Mat extra_rotation = FullRotation(state, warp);

  std::vector<ImVec2> projected(projectable.points.size());
  std::transform(projectable.points.begin(), projectable.points.end(),
                 projected.begin(), [&](const cv::Point2f& point) {
                   auto projected_point = warp.warper->warpPoint(
                       point, camera.k_mat, extra_rotation * camera.r_mat);

                   auto translated = projected_point + projectable.translation;

                   return ImVec2{
                       (translated.x / static_cast<float>(warp.scale.width)) *
                               image.size[0] +
                           image.start[0],
                       (translated.y / static_cast<float>(warp.scale.height)) *
                               image.size[1] +
                           image.start[1]};
                 });
  return projected;
}

DragResult<RotationState> Drag(const RotationWidget& widget,
                               const utils::RectPVf& image,
                               utils::Point2f mouse_pos, bool mouse_clicked,
                               bool mouse_down) {
  auto within_image = [&image](const utils::Point2f& pos) {
    return pos[0] >= image.start[0] &&
           pos[0] <= image.start[0] + image.size[0] &&
           pos[1] >= image.start[1] && pos[1] <= image.start[1] + image.size[1];
  };
  auto new_rotation = widget.rotation;
  bool dragging = false;
  bool mouse_close = false;
  bool finished_dragging = false;
  for (auto& edge : new_rotation.edges) {
    switch (edge.type) {
      case EdgeType::kHorizontal: {
        auto horiz =
            Warp(widget.horizontal_handle, widget.warp, widget.rotation, image);
        edge.mouse_close = IsMouseCloseToPoly(horiz, mouse_pos);
        break;
      }
      case EdgeType::kVertical: {
        auto vertical =
            Warp(widget.vertical_handle, widget.warp, widget.rotation, image);
        edge.mouse_close = IsMouseCloseToPoly(vertical, mouse_pos);
        break;
      }
      case EdgeType::kRoll: {
        edge.mouse_close = !mouse_close && within_image(mouse_pos);
        break;
      }
      default:
        continue;
    }

    if (edge.mouse_close && mouse_clicked) {
      edge.dragging = true;
      new_rotation.mouse_start = mouse_pos;
      new_rotation.yaw_start = new_rotation.yaw;
      new_rotation.pitch_start = new_rotation.pitch;
      new_rotation.roll_start = new_rotation.roll;
    }

    if (edge.dragging && !mouse_down) {
      edge.dragging = false;
      finished_dragging = true;
    }

    dragging |= edge.dragging;
    mouse_close |= edge.mouse_close;
  }

  if (!dragging) {
    return {new_rotation, finished_dragging};
  }

  for (const auto& edge : new_rotation.edges) {
    if (edge.dragging) {
      switch (edge.type) {
        case EdgeType::kHorizontal: {
          new_rotation.pitch =
              new_rotation.pitch_start +
              ComputePitch(new_rotation.mouse_start - mouse_pos, widget.warp,
                           image);
          break;
        }
        case EdgeType::kVertical: {
          new_rotation.yaw = new_rotation.yaw_start +
                             ComputeYaw(new_rotation.mouse_start - mouse_pos,
                                        widget.warp, image);
          break;
        }
        case EdgeType::kRoll: {
          auto roll =
              Warp(widget.roll_handle, widget.warp, widget.rotation, image);
          new_rotation.roll = new_rotation.roll_start +
                              ComputeRoll(new_rotation.mouse_start, mouse_pos,
                                          utils::ToPoint(roll[0]));
          break;
        }
        default:
          continue;
      }
    }
  }
  return {new_rotation, false};
}

void SelectMouseCursor(const widgets::RotationWidget& widget) {
  const int mouse_cursor_selector = std::accumulate(
      widget.rotation.edges.begin(), widget.rotation.edges.end(), 0,
      [](int sum, const widgets::Edge& edge) {
        return sum + (edge.mouse_close || edge.dragging
                          ? static_cast<int>(edge.type)
                          : 0);
      });

  switch (mouse_cursor_selector) {
    case Select(EdgeType::kRoll, EdgeType::kHorizontal):
      [[fallthrough]];
    case Select(EdgeType::kHorizontal):
      ImGui::SetMouseCursor((IsHorizontal(widget.warp.pitch_axis.drag_dir))
                                ? ImGuiMouseCursor_ResizeEW
                                : ImGuiMouseCursor_ResizeNS);
      break;
    case Select(EdgeType::kRoll, EdgeType::kVertical):
      [[fallthrough]];
    case Select(EdgeType::kVertical):
      ImGui::SetMouseCursor((IsHorizontal(widget.warp.yaw_axis.drag_dir))
                                ? ImGuiMouseCursor_ResizeEW
                                : ImGuiMouseCursor_ResizeNS);
      break;
    case Select(EdgeType::kRoll, EdgeType::kHorizontal, EdgeType::kVertical):
      [[fallthrough]];
    case Select(EdgeType::kHorizontal, EdgeType::kVertical):
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
      break;
    case Select(EdgeType::kRoll):
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
      break;
    default:
      ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
      break;
  }
}

}  // namespace xpano::gui::widgets