// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/stitching/detail/warpers.hpp>

#include "xpano/algorithm/algorithm.h"
#include "xpano/gui/widgets/widgets.h"
#include "xpano/utils/rect.h"

namespace xpano::gui::widgets {

struct Projectable {
  int camera_id;
  std::vector<cv::Point2f> points;
  cv::Point2f translation;
};

using Polyline = std::vector<ImVec2>;

struct PreprocessedCamera {
  cv::Mat k_mat;  // in CV_32F as expected by warper functions
  cv::Mat r_mat;
};

constexpr auto DefaultEdgesRotation() {
  return std::array{Edge{EdgeType::kHorizontal}, Edge{EdgeType::kVertical},
                    Edge{EdgeType::kRoll}};
}

struct RotationState {
  float yaw = 0.0f;
  float pitch = 0.0f;
  float roll = 0.0f;

  float yaw_start = 0.0f;
  float pitch_start = 0.0f;
  float roll_start = 0.0f;

  utils::Point2f mouse_start;

  std::array<Edge, 3> edges = DefaultEdgesRotation();
};

struct Axis {
  cv::Mat coords;
  cv::Point2f drag_dir;
  float rot_speed;  // angle per 1 shifted pixel
};

struct StaticWarpData {
  cv::Size scale;
  std::vector<PreprocessedCamera> cameras;
  cv::Ptr<cv::detail::RotationWarper> warper;
  cv::Mat roll_axis;
  Axis pitch_axis;
  Axis yaw_axis;
};

struct RotationWidget {
  Projectable horizontal_handle;
  Projectable vertical_handle;
  Projectable roll_handle;
  std::vector<Projectable> image_borders;

  StaticWarpData warp;
  RotationState rotation;
};

RotationWidget SetupRotationWidget(const algorithm::Cameras& cameras);

DragResult<RotationState> Drag(const RotationWidget& widget,
                               const utils::RectPVf& image,
                               utils::Point2f mouse_pos, bool mouse_clicked,
                               bool mouse_down);

cv::Mat FullRotation(const RotationState& state, const StaticWarpData& warp);

Polyline Warp(const Projectable& projectable, const StaticWarpData& warp,
              const RotationState& state, const utils::RectPVf& image);

void SelectMouseCursor(const widgets::RotationWidget& widget);

}  // namespace xpano::gui::widgets