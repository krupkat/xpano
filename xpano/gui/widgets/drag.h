// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "xpano/gui/widgets/widgets.h"
#include "xpano/utils/rect.h"
#include "xpano/utils/vec.h"

namespace xpano::gui::widgets {

constexpr auto DefaultEdges() {
  return std::array{Edge{EdgeType::kTop}, Edge{EdgeType::kBottom},
                    Edge{EdgeType::kLeft}, Edge{EdgeType::kRight}};
}

struct DraggableWidget {
  utils::RectRRf rect = utils::DefaultCropRect();
  std::array<Edge, 4> edges = DefaultEdges();
};

DragResult<DraggableWidget> Drag(const DraggableWidget& input_widget,
                                 const utils::RectPVf& image,
                                 utils::Point2f mouse_pos, bool mouse_clicked,
                                 bool mouse_down);

void SelectMouseCursor(const widgets::DraggableWidget& crop);

}  // namespace xpano::gui::widgets