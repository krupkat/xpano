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

template <typename... TEdge>
constexpr int Select(TEdge... edges) {
  return (static_cast<int>(edges) + ...);
}

struct Edge {
  EdgeType type;
  bool dragging = false;
  bool mouse_close = false;
};

template <typename TWidget>
struct DragResult {
  TWidget widget;
  bool finished_dragging = false;
};

}  // namespace xpano::gui::widgets