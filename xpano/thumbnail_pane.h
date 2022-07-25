#pragma once

#include <vector>

#include <imgui.h>
#include <SDL.h>

#include "action.h"
#include "image.h"
#include "utils/sdl_.h"

namespace xpano::gui {

class HoverChecker {
 public:
  void SetColor(int img_id);
  void ResetColor(int img_id, bool ctrl_pressed);
  void Highlight(const std::vector<int> &ids, bool allow_modification);

  [[nodiscard]] bool AllowsMofication() const;

  const std::vector<int>& HighlightedIds() const { return highlighted_ids_; }

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
 public:
  explicit ThumbnailPane(SDL_Renderer *renderer);
  void Load(const std::vector<Image> &images);
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

  int scroll_id_ = 0;
  ImVec2 window_size_ = ImVec2(0, 0);
  float last_thumbnail_height_ = 0.0f;
  int resizing_streak_ = 0;

  float last_scroll_x_ = 0.0f;
  float last_scroll_x_max_ = 0.0f;
  float scroll_ratio_ = 0.0f;
  bool rescroll_ = false;

  HoverChecker hover_checker_;
  SDL_Renderer *renderer_;

  ImGuiIO &io_ = ImGui::GetIO();
};

}  // namespace xpano::gui
