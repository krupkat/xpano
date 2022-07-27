#pragma once

#include <imgui.h>

#include "utils/vec.h"

namespace xpano::gui {

struct Coord {
  utils::Ratio2f uv0;
  utils::Ratio2f uv1;
  float aspect;
  int id;
};

}  // namespace xpano::gui
