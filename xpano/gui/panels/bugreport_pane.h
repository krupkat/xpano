#pragma once

namespace xpano::gui {

class BugReportPane {
 public:
  explicit BugReportPane();
  void Draw();
  void Show();

 private:
  bool show_ = false;
};

}  // namespace xpano::gui
