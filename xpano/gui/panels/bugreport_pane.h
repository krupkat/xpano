#pragma once

#include <future>

#include "xpano/utils/text.h"

namespace xpano::gui {

class BugReportPane {
 public:
  explicit BugReportPane();
  void Draw();
  void Show();

 private:
  bool show_ = false;
  int current_license_ = 0;
  std::future<utils::Texts> licenses_future_;
  utils::Texts licenses_;
};

}  // namespace xpano::gui
