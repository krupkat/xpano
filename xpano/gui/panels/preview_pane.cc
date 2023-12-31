// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/panels/preview_pane.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <utility>
#include <vector>

#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/stitching/detail/camera.hpp>
#include <spdlog/spdlog.h>

#include "xpano/constants.h"
#include "xpano/gui/backends/base.h"
#include "xpano/utils/opencv.h"
#include "xpano/utils/vec.h"
#include "xpano/utils/vec_converters.h"

namespace xpano::gui {

namespace {
void DrawMessage(utils::Point2f pos, const std::string& message) {
  if (message.empty()) {
    return;
  }
  const ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
      ImGuiWindowFlags_NoMove;
  ImGui::SetNextWindowPos(utils::ImVec(pos), ImGuiCond_Always,
                          ImVec2(0.0f, 1.0f));
  ImGui::Begin("Overlay", nullptr, window_flags);
  ImGui::TextUnformatted(message.c_str());
  ImGui::End();
}

auto CropRectPP(const utils::RectPVf& image, const utils::RectRRf& crop_rect) {
  return utils::Rect(image.start + image.size * crop_rect.start,
                     image.start + image.size * crop_rect.end);
}

void Overlay(const utils::RectRRf& crop_rect, const utils::RectPVf& image) {
  auto rect = CropRectPP(image, crop_rect);

  const auto transparent_color = ImColor(0, 0, 0, 128);
  auto start = ImVec2(image.start[0], image.start[1]);
  auto end = ImVec2(rect.start[0], image.start[1] + image.size[1]);
  ImGui::GetWindowDrawList()->AddRectFilled(start, end, transparent_color);
  start = ImVec2(rect.end[0], image.start[1]);
  end = ImVec2(image.start[0] + image.size[0], image.start[1] + image.size[1]);
  ImGui::GetWindowDrawList()->AddRectFilled(start, end, transparent_color);
  start = ImVec2(rect.start[0], image.start[1]);
  end = ImVec2(rect.end[0], rect.start[1]);
  ImGui::GetWindowDrawList()->AddRectFilled(start, end, transparent_color);
  start = ImVec2(rect.start[0], rect.end[1]);
  end = ImVec2(rect.end[0], image.start[1] + image.size[1]);
  ImGui::GetWindowDrawList()->AddRectFilled(start, end, transparent_color);

  const auto crop_color = ImColor(255, 255, 255, 255);
  ImGui::GetWindowDrawList()->AddRect(utils::ImVec(rect.start),
                                      utils::ImVec(rect.end), crop_color, 0.0f,
                                      0, 2.0f);
}

void Draw(const Projectable& projectable, const PreprocessedCamera& camera,
          const cv::Ptr<cv::detail::RotationWarper>& warper,
          const cv::Size& scale, const utils::RectPVf& image) {
  const auto color = ImColor(255, 255, 255, 255);

  std::vector<ImVec2> projected(projectable.points.size());
  std::transform(
      projectable.points.begin(), projectable.points.end(), projected.begin(),
      [&](const cv::Point2f& point) {
        auto projected_point =
            warper->warpPoint(point, camera.k_mat, camera.r_mat);

        auto translated = projected_point + projectable.translation;

        return ImVec2{
            (translated.x / static_cast<float>(scale.width)) * image.size[0] +
                image.start[0],
            (translated.y / static_cast<float>(scale.height)) * image.size[1] +
                image.start[1]};
      });

  ImGui::GetWindowDrawList()->AddPolyline(projected.data(), projected.size(),
                                          color, ImDrawFlags_None, 2.0f);
}

void Overlay(const RotationWidget& widget, const utils::RectPVf& image) {
  for (const auto& border : widget.image_borders) {
    const auto& camera = widget.cameras[border.camera_id];
    Draw(border, camera, widget.warper, widget.scale, image);
  }

  {
    const auto& camera = widget.cameras[widget.horizontal_handle.camera_id];
    Draw(widget.horizontal_handle, camera, widget.warper, widget.scale, image);
  }

  {
    const auto& camera = widget.cameras[widget.vertical_handle.camera_id];
    Draw(widget.vertical_handle, camera, widget.warper, widget.scale, image);
  }
}

bool IsMouseCloseToEdge(EdgeType edge_type, const utils::RectPPf& rect,
                        utils::Point2f mouse_pos) {
  auto within_x_bounds = [&rect](const utils::Point2f& mouse_pos) {
    return mouse_pos[0] > rect.start[0] - kCropEdgeTolerance &&
           mouse_pos[0] < rect.end[0] + kCropEdgeTolerance;
  };

  auto within_y_bounds = [&rect](const utils::Point2f& mouse_pos) {
    return mouse_pos[1] > rect.start[1] - kCropEdgeTolerance &&
           mouse_pos[1] < rect.end[1] + kCropEdgeTolerance;
  };

  switch (edge_type) {
    case EdgeType::kTop: {
      return std::abs(mouse_pos[1] - rect.start[1]) < kCropEdgeTolerance &&
             within_x_bounds(mouse_pos);
    }
    case EdgeType::kBottom: {
      return std::abs(mouse_pos[1] - rect.end[1]) < kCropEdgeTolerance &&
             within_x_bounds(mouse_pos);
    }
    case EdgeType::kLeft: {
      return std::abs(mouse_pos[0] - rect.start[0]) < kCropEdgeTolerance &&
             within_y_bounds(mouse_pos);
    }
    case EdgeType::kRight: {
      return std::abs(mouse_pos[0] - rect.end[0]) < kCropEdgeTolerance &&
             within_y_bounds(mouse_pos);
    }
    default: {
      return false;
    }
  }
}

DraggableWidget Drag(const DraggableWidget& input_widget,
                     const utils::RectPVf& image, utils::Point2f mouse_pos,
                     bool mouse_clicked, bool mouse_down) {
  auto widget = input_widget;
  auto rect_window_coords = CropRectPP(image, widget.rect);

  bool dragging = false;
  for (auto& edge : widget.edges) {
    edge.mouse_close =
        IsMouseCloseToEdge(edge.type, rect_window_coords, mouse_pos);
    if (edge.mouse_close && mouse_clicked) {
      edge.dragging = true;
    }
    if (!mouse_down) {
      edge.dragging = false;
    }
    dragging |= edge.dragging;
  }

  if (!dragging) {
    return widget;
  }

  auto new_pos = (mouse_pos - image.start) / image.size;
  auto tolerance =
      utils::Vec2f{static_cast<float>(kCropEdgeTolerance)} / image.size * 10.0f;
  for (const auto& edge : widget.edges) {
    if (edge.dragging) {
      switch (edge.type) {
        case EdgeType::kTop: {
          widget.rect.start[1] =
              std::clamp(new_pos[1], 0.0f, widget.rect.end[1] - tolerance[1]);
          break;
        }
        case EdgeType::kBottom: {
          widget.rect.end[1] =
              std::clamp(new_pos[1], widget.rect.start[1] + tolerance[1], 1.0f);
          break;
        }
        case EdgeType::kLeft: {
          widget.rect.start[0] =
              std::clamp(new_pos[0], 0.0f, widget.rect.end[0] - tolerance[0]);
          break;
        }
        case EdgeType::kRight: {
          widget.rect.end[0] =
              std::clamp(new_pos[0], widget.rect.start[0] + tolerance[0], 1.0f);
          break;
        }
      }
    }
  }

  return widget;
}

template <typename... TEdge>
constexpr int Select(TEdge... edges) {
  return (static_cast<int>(edges) + ...);
}

void SelectMouseCursor(const DraggableWidget& crop) {
  const int mouse_cursor_selector = std::accumulate(
      crop.edges.begin(), crop.edges.end(), 0, [](int sum, const Edge& edge) {
        return sum + (edge.mouse_close || edge.dragging
                          ? static_cast<int>(edge.type)
                          : 0);
      });

  switch (mouse_cursor_selector) {
    case Select(EdgeType::kTop):
      [[fallthrough]];
    case Select(EdgeType::kBottom):
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
      break;
    case Select(EdgeType::kLeft):
      [[fallthrough]];
    case Select(EdgeType::kRight):
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
      break;
    case Select(EdgeType::kBottom, EdgeType::kRight):
      [[fallthrough]];
    case Select(EdgeType::kTop, EdgeType::kLeft): {
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
      break;
    }
    case Select(EdgeType::kBottom, EdgeType::kLeft):
      [[fallthrough]];
    case Select(EdgeType::kTop, EdgeType::kRight): {
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
      break;
    }
    default:
      ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
      break;
  }
}

std::vector<cv::Point2f> PointsOnRectangle(cv::Size size,
                                           int points_per_edge = 10) {
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

std::vector<cv::Point2f> Interpolate(const cv::Point& start,
                                     const cv::Point& end, int num_edges = 10) {
  std::vector<cv::Point2f> points;
  points.reserve(num_edges + 1);
  for (int i = 0; i <= num_edges; i++) {
    float alpha = static_cast<float>(i) / static_cast<float>(num_edges);
    points.push_back(cv::Point2f{start} + alpha * cv::Point2f{end - start});
  }
  return points;
}

std::vector<cv::Point2f> WarpBackpward(
    const std::vector<cv::Point2f>& points, const PreprocessedCamera& camera,
    const cv::Ptr<cv::detail::RotationWarper>& warper) {
  std::vector<cv::Point2f> warped(points.size());
  std::transform(points.begin(), points.end(), warped.begin(),
                 [&](const cv::Point2f& point) {
                   return warper->warpPointBackward(point, camera.k_mat,
                                                    camera.r_mat);
                 });
  return warped;
}

Projectable HorizontalHandle(
    cv::Rect dst_roi, int camera_id, const PreprocessedCamera& camera,
    const cv::Ptr<cv::detail::RotationWarper>& warper) {
  int mid_y = (dst_roi.tl().y + dst_roi.br().y) / 2;

  auto start = cv::Point{dst_roi.tl().x, mid_y};
  auto end = cv::Point{dst_roi.br().x, mid_y};
  auto diff = end - start;

  auto points = Interpolate(start - diff, end + diff);

  return {.camera_id = camera_id,
          .points = WarpBackpward(points, camera, warper),
          .translation = -dst_roi.tl()};
}

Projectable VerticalHandle(cv::Rect dst_roi, int camera_id,
                           const PreprocessedCamera& camera,
                           const cv::Ptr<cv::detail::RotationWarper>& warper) {
  int mid_x = (dst_roi.tl().x + dst_roi.br().x) / 2;

  auto start = cv::Point{mid_x, dst_roi.tl().y};
  auto end = cv::Point{mid_x, dst_roi.br().y};
  auto diff = end - start;

  auto points = Interpolate(start - diff, end + diff);

  return {.camera_id = camera_id,
          .points = WarpBackpward(points, camera, warper),
          .translation = -dst_roi.tl()};
}

Projectable RollHandle(int camera_id) { return {}; }

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

RotationWidget SetupRotationWidget(const algorithm::Cameras& cameras) {
  int num_cameras = static_cast<int>(cameras.cameras.size());

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

  int camera_id = num_cameras / 2;
  auto& camera = preprocessed_cameras[camera_id];

  auto vertical_handle =
      VerticalHandle(dst_roi, camera_id, camera, cameras.warp_helper.warper);
  auto horizontal_handle =
      HorizontalHandle(dst_roi, camera_id, camera, cameras.warp_helper.warper);

  return {.horizontal_handle = std::move(horizontal_handle),
          .vertical_handle = std::move(vertical_handle),
          .roll_handle = RollHandle(camera_id),
          .image_borders = std::move(projectables),
          .scale = dst_roi.size(),
          .cameras = std::move(preprocessed_cameras),
          .warper = cameras.warp_helper.warper};
}

}  // namespace

PreviewPane::PreviewPane(backends::Base* backend) : backend_(backend) {
  std::iota(zoom_levels_.begin(), zoom_levels_.end(), -1.0f);
  std::transform(zoom_levels_.begin(), zoom_levels_.end(), zoom_levels_.begin(),
                 [](float exp) { return std::pow(kZoomFactor, exp); });
};

float PreviewPane::Zoom() const { return zoom_; }

bool PreviewPane::IsZoomed() const { return zoom_id_ != 1; }

void PreviewPane::ZoomIn() {
  if (crop_mode_ != CropMode::kEnabled && zoom_id_ < kZoomLevels - 1) {
    zoom_id_++;
  }
}

void PreviewPane::ZoomOut() {
  if (zoom_id_ > 1) {
    zoom_id_--;
  }
}

void PreviewPane::AdvanceZoom() {
  const float zoom_epsilon = zoom_ * kZoomSpeed;
  if (std::abs(zoom_ - zoom_levels_[zoom_id_]) > zoom_epsilon) {
    zoom_ = zoom_ > zoom_levels_[zoom_id_] ? zoom_ - zoom_epsilon
                                           : zoom_ + zoom_epsilon;
  } else {
    zoom_ = zoom_levels_[zoom_id_];
  }
}

void PreviewPane::ResetZoom(int target_level) {
  zoom_id_ = target_level;
  zoom_ = zoom_levels_[target_level];
  screen_offset_ = utils::Ratio2f{0.5f, 0.5f};
  image_offset_ = utils::Ratio2f{0.5f, 0.5f};
}

void PreviewPane::Load(cv::Mat image, ImageType image_type) {
  Reset();
  Reload(std::move(image), image_type);
}

void PreviewPane::Reload(cv::Mat image, ImageType image_type) {
  auto texture_size = utils::Vec2i{kLoupeSize};
  if (!tex_) {
    tex_ = backend_->CreateTexture(texture_size);
  }

  const int larger_dim = image.size[0] > image.size[1] ? 0 : 1;

  cv::Mat resized;
  utils::Ratio2f coord_uv;
  if (image.size[larger_dim] > kLoupeSize) {
    if (const float aspect = utils::ToIntVec(image.size).Aspect();
        aspect >= 1.0f) {
      coord_uv = {1.0f, 1.0f / aspect};
    } else {
      coord_uv = {1.0f * aspect, 1.0f};
    }
    auto size = utils::ToIntVec(texture_size * coord_uv);
    cv::resize(image, resized, utils::CvSize(size), 0, 0, cv::INTER_AREA);
  } else {
    resized = image;
    coord_uv = utils::ToIntVec(image.size) / texture_size;
  }
  backend_->UpdateTexture(tex_.get(), resized);
  tex_coord_ = coord_uv;

  image_type_ = image_type;
  if (image_type == ImageType::kPanoFullRes) {
    full_resolution_pano_ = image;
  }
}

void PreviewPane::Reset() {
  ResetZoom();
  image_type_ = ImageType::kNone;
  crop_mode_ = CropMode::kInitial;
  crop_widget_ = {};
  rotate_mode_ = RotateMode::kDisabled;
  rotate_widget_ = {};
  suggested_crop_ = DefaultCropRect();
  full_resolution_pano_ = cv::Mat{};
}

void PreviewPane::Draw(const std::string& message) {
  ImGui::Begin("Preview");
  auto window = utils::Rect(utils::ToPoint(ImGui::GetCursorScreenPos()),
                            utils::ToVec(ImGui::GetContentRegionAvail()));
  DrawMessage(window.start + utils::Vec2f{0.0f, window.size[1]}, message);

  if (tex_ && image_type_ != ImageType::kNone) {
    auto mid = window.start + window.size / 2.0f;

    float image_aspect = tex_coord_.Aspect();
    if (crop_mode_ == CropMode::kDisabled) {
      image_aspect *= utils::Aspect(crop_widget_.rect);
    };

    auto image_size =
        window.size.Aspect() < image_aspect
            ? utils::Vec2f{window.size[0], window.size[0] / image_aspect}
            : utils::Vec2f{window.size[1] * image_aspect, window.size[1]};

    auto image = utils::Rect(mid - image_size / 2.0f, image_size);
    if (AdvanceZoom(); IsZoomed()) {
      image = utils::Rect(window.start + window.size * screen_offset_ -
                              image_size * image_offset_ * Zoom(),
                          image_size * Zoom());
    }

    HandleInputs(window, image);

    auto tex_coords =
        (crop_mode_ == CropMode::kEnabled || crop_mode_ == CropMode::kInitial)
            ? utils::Rect(utils::Ratio2f{0.0f}, tex_coord_)
            : utils::Rect(tex_coord_ * crop_widget_.rect.start,
                          tex_coord_ * crop_widget_.rect.end);

    ImGui::GetWindowDrawList()->AddImage(tex_.get(), utils::ImVec(image.start),
                                         utils::ImVec(image.start + image.size),
                                         utils::ImVec(tex_coords.start),
                                         utils::ImVec(tex_coords.end));

    if (crop_mode_ == CropMode::kEnabled) {
      Overlay(crop_widget_.rect, image);
    }

    if (rotate_mode_ == RotateMode::kEnabled) {
      Overlay(rotate_widget_, image);
    }
  }
  ImGui::End();
}

void PreviewPane::HandleInputs(const utils::RectPVf& window,
                               const utils::RectPVf& image) {
  // Let the crop widget take events from the whole window
  // to be able to set the correct cursor icon

  if (crop_mode_ == CropMode::kEnabled ||
      rotate_mode_ == RotateMode::kEnabled) {
    const bool mouse_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    auto mouse_pos = utils::ToPoint(ImGui::GetMousePos());

    if (crop_mode_ == CropMode::kEnabled) {
      crop_widget_ =
          Drag(crop_widget_, image, mouse_pos, mouse_clicked, mouse_down);
      SelectMouseCursor(crop_widget_);
      return;
    }

    if (rotate_mode_ == RotateMode::kEnabled) {
      return;
    }
  }

  if (!ImGui::IsWindowHovered()) {
    return;
  }

  // Zoom + pan only when not cropping
  const bool mouse_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
  const bool mouse_dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
  const float mouse_wheel = ImGui::GetIO().MouseWheel;

  if (mouse_clicked || mouse_dragging || mouse_wheel != 0) {
    auto mouse_pos = utils::ToPoint(ImGui::GetMousePos());
    screen_offset_ = (mouse_pos - window.start) / window.size;
    if (!mouse_dragging) {
      image_offset_ = (mouse_pos - image.start) / image.size;
    }
  }
  if (mouse_wheel > 0) {
    ZoomIn();
  }
  if (mouse_wheel < 0) {
    ZoomOut();
  }
}

ImageType PreviewPane::Type() const { return image_type_; }

void PreviewPane::ToggleCrop() {
  if (image_type_ != ImageType::kPanoFullRes) {
    return;
  }

  switch (crop_mode_) {
    case CropMode::kInitial:
      ResetZoom();
      EndRotate();
      crop_widget_.rect = suggested_crop_;
      crop_mode_ = CropMode::kEnabled;
      break;
    case CropMode::kEnabled:
      crop_mode_ = CropMode::kDisabled;
      break;
    case CropMode::kDisabled:
      ResetZoom();
      EndRotate();
      crop_mode_ = CropMode::kEnabled;
      break;
    default:
      break;
  }
}

void PreviewPane::ToggleRotate() {
  switch (rotate_mode_) {
    case RotateMode::kEnabled:
      ResetZoom(1);
      rotate_mode_ = RotateMode::kDisabled;
      break;
    case RotateMode::kDisabled:
      if (cameras_) {
        ResetZoom(0);
        EndCrop();
        rotate_widget_ = SetupRotationWidget(*cameras_);
        rotate_mode_ = RotateMode::kEnabled;
      } else {
        spdlog::warn("Cannot enable rotate mode, missing camera parameters.");
      }
      break;
    default:
      break;
  }
}

void PreviewPane::EndCrop() {
  if (crop_mode_ == CropMode::kEnabled) {
    crop_mode_ = CropMode::kDisabled;
  }
}

void PreviewPane::EndRotate() {
  if (rotate_mode_ == RotateMode::kEnabled) {
    rotate_mode_ = RotateMode::kDisabled;
  }
}

cv::Mat PreviewPane::Image() const { return full_resolution_pano_; }

utils::RectRRf PreviewPane::CropRect() const { return crop_widget_.rect; }

void PreviewPane::SetSuggestedCrop(const utils::RectRRf& rect) {
  suggested_crop_ = rect;
}

void PreviewPane::SetCameras(const algorithm::Cameras& cameras) {
  cameras_ = cameras;
}

}  // namespace xpano::gui
