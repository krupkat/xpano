#pragma once

#include <vector>

#include <imgui.h>
#include <SDL.h>

#include "algorithm/image.h"
#include "gui/action.h"
#include "utils/sdl_.h"
#include "utils/vec.h"

namespace xpano::gui {

class HoverChecker {
 public:
  void SetColor(int img_id);
  void ResetColor(int img_id, bool ctrl_pressed);
  void Highlight(const std::vector<int> &ids, bool allow_modification);

  [[nodiscard]] bool AllowsMofication() const;

 private:
  [[nodiscard]] bool WasHovered(int img_id) const;
  void RecordHover(int img_id);
  void ResetHover();

  int hover_id_ = -1;
  int styles_pushed_ = 0;
  bool allow_modification_ = false;
  std::vector<int> highlighted_ids_;
};

class ThumbnailPane {
  struct Coord {
    utils::Ratio2f uv0;
    utils::Ratio2f uv1;
    float aspect;
  };

 public:
  explicit ThumbnailPane(SDL_Renderer *renderer);
  void Load(const std::vector<algorithm::Image> &images);
  [[nodiscard]] bool Loaded() const;

  Action Draw();
  void SetScrollX(int img_id);
  void SetScrollX(int id1, int id2);
  void SetScrollX(const std::vector<int> &ids);

  void Highlight(int img_id);
  void Highlight(int id1, int id2);
  void Highlight(const std::vector<int> &ids, bool allow_modification = false);

  void Reset();

 private:
  utils::sdl::Texture tex_;
  std::vector<Coord> coords_;
  std::vector<float> scroll_;

  HoverChecker hover_checker_;
  SDL_Renderer *renderer_;

  ImGuiIO &io_ = ImGui::GetIO();
};

}  // namespace xpano::gui
