// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/widgets/drag.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include <imgui.h>

#include "xpano/constants.h"

namespace xpano::gui::widgets {

namespace {
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
}  // namespace

DragResult<DraggableWidget> Drag(const DraggableWidget& input_widget,
                                 const utils::RectPVf& image,
                                 utils::Point2f mouse_pos, bool mouse_clicked,
                                 bool mouse_down) {
  auto widget = input_widget;
  auto rect_window_coords = CropRectPP(image, widget.rect);

  bool dragging = false;
  bool finished_dragging = false;
  for (auto& edge : widget.edges) {
    edge.mouse_close =
        IsMouseCloseToEdge(edge.type, rect_window_coords, mouse_pos);
    if (edge.mouse_close && mouse_clicked) {
      edge.dragging = true;
    }
    if (edge.dragging && !mouse_down) {
      edge.dragging = false;
      finished_dragging = true;
    }
    dragging |= edge.dragging;
  }

  if (!dragging) {
    return {widget, finished_dragging};
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

  return {widget, false};
}

void SelectMouseCursor(const widgets::DraggableWidget& crop) {
  const int mouse_cursor_selector =
      std::accumulate(crop.edges.begin(), crop.edges.end(), 0,
                      [](int sum, const widgets::Edge& edge) {
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

}  // namespace xpano::gui::widgets