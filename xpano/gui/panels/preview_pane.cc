#include "xpano/gui/panels/preview_pane.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <spdlog/spdlog.h>

#include "xpano/constants.h"
#include "xpano/gui/backends/base.h"
#include "xpano/utils/vec.h"
#include "xpano/utils/vec_converters.h"

namespace xpano::gui {

namespace {
void DrawMessage(utils::Point2f pos, const std::string& message) {
  if (message.empty()) {
    return;
  }
  ImGuiWindowFlags window_flags =
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

void DrawCropRectangle(utils::Ratio2f crop_start, utils::Ratio2f crop_end,
                       utils::Point2f image_start, utils::Vec2f image_size) {
  auto top_left = image_start + image_size * crop_start;
  auto bottom_right = image_start + image_size * crop_end;
  const auto crop_color = ImColor(255, 255, 255, 255);
  ImGui::GetWindowDrawList()->AddRect(utils::ImVec(top_left),
                                      utils::ImVec(bottom_right), crop_color,
                                      0.0f, 0, 2.0f);
}

bool IsMouseCloseToEdge(EdgeType edge_type, utils::Point2f top_left,
                        utils::Point2f bottom_right, utils::Point2f mouse_pos) {
  switch (edge_type) {
    case EdgeType::kTop: {
      return std::abs(mouse_pos[1] - top_left[1]) < kCropEdgeTolerance &&
             mouse_pos[0] > top_left[0] && mouse_pos[0] < bottom_right[0];
    }
    case EdgeType::kBottom: {
      return std::abs(mouse_pos[1] - bottom_right[1]) < kCropEdgeTolerance &&
             mouse_pos[0] > top_left[0] && mouse_pos[0] < bottom_right[0];
    }
    case EdgeType::kLeft: {
      return std::abs(mouse_pos[0] - top_left[0]) < kCropEdgeTolerance &&
             mouse_pos[1] > top_left[1] && mouse_pos[1] < bottom_right[1];
    }
    case EdgeType::kRight: {
      return std::abs(mouse_pos[0] - bottom_right[0]) < kCropEdgeTolerance &&
             mouse_pos[1] > top_left[1] && mouse_pos[1] < bottom_right[1];
    }
    default: {
      return false;
    }
  }
}

CropState EditCropRectangle(const CropState& input_crop,
                            utils::Point2f image_start, utils::Vec2f image_size,
                            utils::Point2f mouse_pos, bool mouse_clicked,
                            bool mouse_down) {
  auto crop = input_crop;
  auto top_left = image_start + image_size * crop.start;
  auto bottom_right = image_start + image_size * crop.end;

  bool dragging = false;
  for (auto& edge : crop.edges) {
    edge.mouse_close =
        IsMouseCloseToEdge(edge.type, top_left, bottom_right, mouse_pos);
    if (edge.mouse_close && mouse_clicked) {
      edge.dragging = true;
    }
    if (!mouse_down) {
      edge.dragging = false;
    }
    dragging |= edge.dragging;
  }

  if (!dragging) {
    return crop;
  }

  auto new_pos = (mouse_pos - image_start) / image_size;
  auto tolerance =
      utils::Vec2f{static_cast<float>(kCropEdgeTolerance)} / image_size * 10.0f;
  for (const auto& edge : crop.edges) {
    if (edge.dragging) {
      switch (edge.type) {
        case EdgeType::kTop: {
          crop.start[1] =
              std::clamp(new_pos[1], 0.0f, crop.end[1] - tolerance[1]);
          break;
        }
        case EdgeType::kBottom: {
          crop.end[1] =
              std::clamp(new_pos[1], crop.start[1] + tolerance[1], 1.0f);
          break;
        }
        case EdgeType::kLeft: {
          crop.start[0] =
              std::clamp(new_pos[0], 0.0f, crop.end[0] - tolerance[0]);
          break;
        }
        case EdgeType::kRight: {
          crop.end[0] =
              std::clamp(new_pos[0], crop.start[0] + tolerance[0], 1.0f);
          break;
        }
      }
    }
  }

  return crop;
}

template <typename... TEdge>
constexpr int Select(TEdge... edges) {
  return (static_cast<int>(edges) + ...);
}

void SelectMouseCursor(const CropState& crop) {
  int mouse_cursor_selector = std::accumulate(
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

}  // namespace

PreviewPane::PreviewPane(backends::Base* backend) : backend_(backend) {
  std::iota(zoom_levels_.begin(), zoom_levels_.end(), 0.0f);
  std::transform(zoom_levels_.begin(), zoom_levels_.end(), zoom_levels_.begin(),
                 [](float exp) { return std::pow(kZoomFactor, exp); });
};

float PreviewPane::Zoom() const { return zoom_; }

bool PreviewPane::IsZoomed() const { return zoom_ != 1.0f; }

void PreviewPane::ZoomIn() {
  if (!crop_.editing && zoom_id_ < kZoomLevels - 1) {
    zoom_id_++;
  }
}

void PreviewPane::ZoomOut() {
  if (zoom_id_ > 0) {
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

void PreviewPane::ResetZoom() {
  zoom_id_ = 0;
  zoom_ = 1.0f;
}

void PreviewPane::Load(cv::Mat image, ImageType image_type) {
  Reset();
  auto texture_size = utils::Vec2i{kLoupeSize};
  if (!tex_) {
    tex_ = backend_->CreateTexture(texture_size);
  }

  int larger_dim = image.size[0] > image.size[1] ? 0 : 1;

  cv::Mat resized;
  utils::Ratio2f coord_uv;
  if (image.size[larger_dim] > kLoupeSize) {
    if (float aspect = utils::ToIntVec(image.size).Aspect(); aspect >= 1.0f) {
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
  crop_ = {};
  full_resolution_pano_ = {};
}

void PreviewPane::Draw(const std::string& message) {
  ImGui::Begin("Preview");

  auto window_start = utils::ToPoint(ImGui::GetCursorScreenPos());
  auto available_size = utils::ToVec(ImGui::GetContentRegionAvail());
  DrawMessage(window_start + utils::Vec2f{0.0f, available_size[1]}, message);

  if (tex_ && image_type_ != ImageType::kNone) {
    auto mid = window_start + available_size / 2.0f;

    float image_aspect = tex_coord_.Aspect();
    if (!crop_.editing) {
      image_aspect *= (crop_.end - crop_.start).Aspect();
    };

    auto image_size =
        available_size.Aspect() < image_aspect
            ? utils::Vec2f{available_size[0], available_size[0] / image_aspect}
            : utils::Vec2f{available_size[1] * image_aspect, available_size[1]};

    auto p_min = mid - image_size / 2.0f;
    auto p_max = mid + image_size / 2.0f;
    if (AdvanceZoom(); IsZoomed()) {
      p_min = window_start + available_size * screen_offset_ -
              image_size * image_offset_ * Zoom();
      p_max = p_min + image_size * Zoom();
    }

    HandleInputs(window_start, available_size, p_min, image_size);

    if (crop_.editing) {
      ImGui::GetWindowDrawList()->AddImage(
          tex_.get(), utils::ImVec(p_min), utils::ImVec(p_max),
          ImVec2(0.0f, 0.0f), utils::ImVec(tex_coord_));

      DrawCropRectangle(crop_.start, crop_.end, p_min, image_size);
    } else {
      ImGui::GetWindowDrawList()->AddImage(
          tex_.get(), utils::ImVec(p_min), utils::ImVec(p_max),
          utils::ImVec(tex_coord_ * crop_.start),
          utils::ImVec(tex_coord_ * crop_.end));
    }
  }
  ImGui::End();
}

void PreviewPane::HandleInputs(const utils::Point2f& window_start,
                               const utils::Vec2f& window_size,
                               const utils::Point2f& image_start,
                               const utils::Vec2f& image_size) {
  // Let the crop widget take events from the whole window
  // to be able to set the correct cursor icon
  if (crop_.editing) {
    bool mouse_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    auto mouse_pos = utils::ToPoint(ImGui::GetMousePos());

    crop_ = EditCropRectangle(crop_, image_start, image_size, mouse_pos,
                              mouse_clicked, mouse_down);
    SelectMouseCursor(crop_);
    return;
  }

  if (!ImGui::IsWindowHovered()) {
    return;
  }

  // Zoom + pan only when not cropping
  bool mouse_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
  bool mouse_dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
  float mouse_wheel = ImGui::GetIO().MouseWheel;

  if (mouse_clicked || mouse_dragging || mouse_wheel != 0) {
    auto mouse_pos = utils::ToPoint(ImGui::GetMousePos());
    screen_offset_ = (mouse_pos - window_start) / window_size;
    if (!mouse_dragging) {
      image_offset_ = (mouse_pos - image_start) / Zoom() / image_size;
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

void PreviewPane::Crop() {
  if (image_type_ != ImageType::kPanoFullRes) {
    return;
  }

  crop_.editing = !crop_.editing;
  ResetZoom();
}

}  // namespace xpano::gui
