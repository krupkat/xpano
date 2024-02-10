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
#include <spdlog/spdlog.h>

#include "xpano/constants.h"
#include "xpano/gui/action.h"
#include "xpano/gui/backends/base.h"
#include "xpano/gui/widgets/widgets.h"
#include "xpano/utils/rect.h"
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

void Draw(const widgets::Polyline& poly, const utils::RectPVf& image) {
  const auto color = ImColor(255, 255, 255, 255);

  auto within_image = [&image](const ImVec2& point) {
    return point.x >= image.start[0] &&
           point.x <= image.start[0] + image.size[0] &&
           point.y >= image.start[1] &&
           point.y <= image.start[1] + image.size[1];
  };

  auto start = poly.begin();
  auto end = poly.begin();

  while (end != poly.end()) {
    while (end != poly.end() && !within_image(*end)) {
      end++;
    }
    if (end == poly.end()) {
      break;
    }
    start = end;
    while (end != poly.end() && within_image(*end)) {
      end++;
    }

    ImGui::GetWindowDrawList()->AddPolyline(&(*start),
                                            static_cast<int>(end - start),
                                            color, ImDrawFlags_None, 2.0f);
  }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): fixme
void Overlay(const widgets::RotationWidget& widget, const utils::RectPVf& image,
             const utils::RectPVf& window) {
  std::vector<widgets::Polyline> polys;
  polys.reserve(widget.image_borders.size() + 2);

  for (const auto& border : widget.image_borders) {
    polys.push_back(Warp(border, widget.warp, widget.rotation, image));
  }

  polys.push_back(
      Warp(widget.horizontal_handle, widget.warp, widget.rotation, image));
  polys.push_back(
      Warp(widget.vertical_handle, widget.warp, widget.rotation, image));

  for (const auto& poly : polys) {
    Draw(poly, window);
  }
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
  suggested_crop_ = utils::DefaultCropRect();
  full_resolution_pano_ = cv::Mat{};
}

Action PreviewPane::Draw(const std::string& message) {
  Action action{};
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

    action |= HandleInputs(window, image);

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
      Overlay(rotate_widget_, image, window);
    }
  }
  ImGui::End();
  return action;
}

Action PreviewPane::HandleInputs(const utils::RectPVf& window,
                                 const utils::RectPVf& image) {
  // Let the crop widget take events from the whole window
  // to be able to set the correct cursor icon

  if (crop_mode_ == CropMode::kEnabled ||
      rotate_mode_ == RotateMode::kEnabled) {
    const bool mouse_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    auto mouse_pos = utils::ToPoint(ImGui::GetMousePos());

    if (crop_mode_ == CropMode::kEnabled) {
      auto drag_result =
          Drag(crop_widget_, image, mouse_pos, mouse_clicked, mouse_down);
      crop_widget_ = drag_result.widget;
      SelectMouseCursor(crop_widget_);
      if (drag_result.finished_dragging) {
        return Action{.type = ActionType::kSaveCrop,
                      .extra = CropExtra{.crop_rect = crop_widget_.rect}};
      }
      return {};
    }

    if (rotate_mode_ == RotateMode::kEnabled) {
      auto [new_state, finished_dragging] =
          Drag(rotate_widget_, image, mouse_pos, mouse_clicked, mouse_down);
      rotate_widget_.rotation = new_state;
      SelectMouseCursor(rotate_widget_);
      if (finished_dragging) {
        return Action{.type = ActionType::kRotate,
                      .delayed = true,
                      .extra = RotateExtra{
                          .rotation_matrix = FullRotation(
                              rotate_widget_.rotation, rotate_widget_.warp)}};
      }
      return {};
    }
  }

  if (!ImGui::IsWindowHovered()) {
    return {};
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
  return {};
}

ImageType PreviewPane::Type() const { return image_type_; }

Action PreviewPane::ToggleCrop() {
  if (image_type_ != ImageType::kPanoFullRes and
      image_type_ != ImageType::kPanoPreview) {
    return {};
  }

  switch (crop_mode_) {
    case CropMode::kInitial:
      ResetZoom();
      EndRotate();
      crop_widget_.rect = suggested_crop_;
      crop_mode_ = CropMode::kEnabled;
      return Action{.type = ActionType::kSaveCrop,
                    .delayed = true,
                    .extra = CropExtra{.crop_rect = crop_widget_.rect}};
    case CropMode::kEnabled:
      crop_mode_ = CropMode::kDisabled;
      break;
    case CropMode::kDisabled: {
      Action action = {};
      if (IsRotateEnabled()) {
        // setting auto crop when going directly from rotate to crop mode
        crop_widget_.rect = suggested_crop_;
        action = {.type = ActionType::kSaveCrop,
                  .delayed = true,
                  .extra = CropExtra{.crop_rect = crop_widget_.rect}};
      }
      ResetZoom();
      EndRotate();
      crop_mode_ = CropMode::kEnabled;
      return action;
    }
    default:
      break;
  }
  return {};
}

bool PreviewPane::IsRotateEnabled() const {
  return rotate_mode_ == RotateMode::kEnabled;
}

Action PreviewPane::ToggleRotate() {
  switch (rotate_mode_) {
    case RotateMode::kEnabled: {
      ResetZoom(1);
      rotate_mode_ = RotateMode::kDisabled;
      return Action{.type = ActionType::kRecrop, .delayed = true};
    }
    case RotateMode::kDisabled: {
      if (cameras_) {
        ResetZoom(0);
        EndCrop();
        rotate_mode_ = RotateMode::kEnabled;
        crop_widget_.rect = utils::DefaultCropRect();
      } else {
        spdlog::warn("Cannot enable rotate mode, missing camera parameters.");
      }
      break;
    }
    default:
      break;
  }

  return {};
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

void PreviewPane::ResetCrop(const utils::RectRRf& rect) {
  crop_mode_ = CropMode::kInitial;
  crop_widget_ = {};
  suggested_crop_ = rect;
}

void PreviewPane::ForceCrop(const utils::RectRRf& rect) {
  crop_widget_.rect = rect;
  if (crop_mode_ == CropMode::kInitial) {
    crop_mode_ = CropMode::kDisabled;
  }
}

void PreviewPane::SetSuggestedCrop(const utils::RectRRf& rect) {
  suggested_crop_ = rect;
}

void PreviewPane::SetCameras(const algorithm::Cameras& cameras) {
  cameras_ = cameras;
  rotate_widget_ = widgets::SetupRotationWidget(*cameras_);
}

}  // namespace xpano::gui
