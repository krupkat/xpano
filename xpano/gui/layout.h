#pragma once

namespace xpano::gui {

class Layout {
 public:
  void Begin();

  void ToggleLogger();
  [[nodiscard]] bool ShowLogger() const;

 private:
  bool show_logger_ = false;
};

}  // namespace xpano::gui
