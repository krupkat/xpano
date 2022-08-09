#pragma once

namespace xpano::gui {

class Layout {
 public:
  void Begin();

  void ToggleDebugInfo();
  [[nodiscard]] bool ShowDebugInfo() const;

 private:
  bool show_debug_info_ = false;
};

}  // namespace xpano::gui
