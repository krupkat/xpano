#pragma once

#include <vector>
#include <future>

#include "utils/text.h"

namespace xpano::gui {

class AboutPane {
 public:
  explicit AboutPane(std::future<std::vector<utils::Text>> licenses);
  void Draw();
  void Show();

 private:
  bool show_ = false;
  int current_license_ = 0;
  std::future<std::vector<utils::Text>> licenses_future_;
  std::vector<utils::Text> licenses_;
};

}  // namespace xpano::gui
