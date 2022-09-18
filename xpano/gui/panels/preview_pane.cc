#include "xpano/gui/panels/preview_pane.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

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

auto CropRectPP(const utils::RectPVf& image, const utils::RectRRf& crop_rect) {
  return utils::Rect(image.start + image.size * crop_rect.start,
                     image.start + image.size * crop_rect.end);
}

void Overlay(const utils::RectRRf& crop_rect, const utils::RectPVf& image) {
  auto rect = CropRectPP(image, crop_rect);
  const auto crop_color = ImColor(255, 255, 255, 255);
  ImGui::GetWindowDrawList()->AddRect(utils::ImVec(rect.start),
                                      utils::ImVec(rect.end), crop_color, 0.0f,
                                      0, 2.0f);
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
  if (crop_mode_ != CropMode::kEnabled && zoom_id_ < kZoomLevels - 1) {
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
  crop_mode_ = CropMode::kInitial;
  crop_widget_ = {};
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
  }
  ImGui::End();
}

void PreviewPane::HandleInputs(const utils::RectPVf& window,
                               const utils::RectPVf& image) {
  // Let the crop widget take events from the whole window
  // to be able to set the correct cursor icon
  if (crop_mode_ == CropMode::kEnabled) {
    bool mouse_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    auto mouse_pos = utils::ToPoint(ImGui::GetMousePos());

    crop_widget_ =
        Drag(crop_widget_, image, mouse_pos, mouse_clicked, mouse_down);
    SelectMouseCursor(crop_widget_);
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
      crop_widget_.rect = suggested_crop_;
      crop_mode_ = CropMode::kEnabled;
      break;
    case CropMode::kEnabled:
      crop_mode_ = CropMode::kDisabled;
      break;
    case CropMode::kDisabled:
      ResetZoom();
      crop_mode_ = CropMode::kEnabled;
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

cv::Mat PreviewPane::Image() const { return full_resolution_pano_; }

utils::RectRRf PreviewPane::CropRect() const { return crop_widget_.rect; }

void PreviewPane::SetSuggestedCrop(const utils::RectRRf& rect) {
  suggested_crop_ = rect;
}

}  // namespace xpano::gui
