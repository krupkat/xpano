#pragma once

#include <imgui.h>

namespace xpano::gui {

struct Coord {
  ImVec2 uv0;
  ImVec2 uv1;
  float aspect;
  int id;
};

}  // namespace xpano
