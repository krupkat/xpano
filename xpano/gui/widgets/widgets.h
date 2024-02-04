// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <vector>

#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/stitching/detail/warpers.hpp>

#include "xpano/algorithm/algorithm.h"
#include "xpano/utils/rect.h"

namespace xpano::gui::widgets {
enum class EdgeType : int {
  kTop = 1,
  kBottom = 2,
  kLeft = 4,
  kRight = 8,
  kHorizontal = 16,
  kVertical = 32,
  kRoll = 64
};

struct Edge {
  EdgeType type;
  bool dragging = false;
  bool mouse_close = false;
};

constexpr auto DefaultEdges() {
  return std::array{Edge{EdgeType::kTop}, Edge{EdgeType::kBottom},
                    Edge{EdgeType::kLeft}, Edge{EdgeType::kRight}};
}

struct DraggableWidget {
  utils::RectRRf rect = utils::DefaultCropRect();
  std::array<Edge, 4> edges = DefaultEdges();
};

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

struct AxisWithSpeed {
  cv::Mat coords;
  float rot_speed;  // angle per 1 shifted pixel
};

struct StaticWarpData {
  cv::Size scale;
  std::vector<PreprocessedCamera> cameras;
  cv::Ptr<cv::detail::RotationWarper> warper;
  cv::Mat roll_axis;
  AxisWithSpeed pitch_axis;
  AxisWithSpeed yaw_axis;
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

template <typename TWidget>
struct DragResult {
  TWidget widget;
  bool finished_dragging = false;
};

DragResult<DraggableWidget> Drag(const DraggableWidget& input_widget,
                                 const utils::RectPVf& image,
                                 utils::Point2f mouse_pos, bool mouse_clicked,
                                 bool mouse_down);

DragResult<RotationState> Drag(const RotationWidget& widget,
                               const utils::RectPVf& image,
                               utils::Point2f mouse_pos, bool mouse_clicked,
                               bool mouse_down);

cv::Mat FullRotation(const RotationState& state, const StaticWarpData& warp);

Polyline Warp(const Projectable& projectable, const StaticWarpData& warp,
              const RotationState& state, const utils::RectPVf& image);

}  // namespace xpano::gui::widgets