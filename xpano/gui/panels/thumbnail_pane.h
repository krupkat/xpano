#pragma once

#include <vector>

#include <imgui.h>

#include "xpano/algorithm/image.h"
#include "xpano/constants.h"
#include "xpano/gui/action.h"
#include "xpano/gui/backends/base.h"
#include "xpano/utils/vec.h"

namespace xpano::gui {

class HoverChecker {
 public:
  void SetColor(int img_id);
  void ResetColor(int img_id, bool ctrl_pressed);
  void Highlight(const std::vector<int> &ids);
  void DisableHighlight();

 private:
  [[nodiscard]] bool WasHovered(int img_id) const;
  void RecordHover(int img_id);
  void ResetHover();

  int hover_id_ = -1;
  int styles_pushed_ = 0;
  std::vector<int> highlighted_ids_;
};

class AutoScroller {
  enum class ScrollType {
    kNone,
    kRatio,
    kAbsolute,
  };

 public:
  void SetScrollTargetCurrentRatio();
  void SetScrollTargetRelative(float scroll_value);
  [[nodiscard]] bool NeedsRescroll() const;
  void Rescroll();

 private:
  ScrollType scroll_type_ = ScrollType::kNone;
  float scroll_target_ = 0.0f;
};

class ResizeChecker {
 public:
  enum class Status {
    kIdle,
    kResizing,
    kResized,
  };

  explicit ResizeChecker(int delay = kResizingDelayFrames);
  Status Check(ImVec2 window_size);

 private:
  const int delay_;

  int resizing_streak_ = 0;
  ImVec2 window_size_ = {0, 0};
};

class ThumbnailPane {
  struct Coord {
    utils::Ratio2f uv0;
    utils::Ratio2f uv1;
    float aspect;
  };

 public:
  explicit ThumbnailPane(backends::Base *backend);
  void Load(const std::vector<algorithm::Image> &images);
  [[nodiscard]] bool Loaded() const;

  Action Draw();

  void ThumbnailTooltip(const std::vector<int> &images) const;

  void SetScrollX(int img_id);
  void SetScrollX(int id1, int id2);
  void SetScrollX(const std::vector<int> &ids);

  void Highlight(int img_id);
  void Highlight(int id1, int id2);
  void Highlight(const std::vector<int> &ids);
  void DisableHighlight();

  void Reset();

 private:
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  bool ThumbnailButton(int img_id) const;

  std::vector<Coord> coords_;
  std::vector<float> scroll_;

  AutoScroller auto_scroller_;
  ResizeChecker resize_checker_;

  float thumbnail_height_ = 0.0f;

  HoverChecker hover_checker_;

  backends::Texture tex_;
  backends::Base *backend_;

  ImGuiIO &io_ = ImGui::GetIO();
};

}  // namespace xpano::gui
