#pragma once

#include <vector>

#include "utils/text.h"

namespace xpano::gui {

class AboutPane {
 public:
  explicit AboutPane(std::vector<utils::Text> licenses);
  void Draw();
  void Show();

 private:
  bool show_ = false;
  int current_license_ = 0;
  std::vector<utils::Text> licenses_;
};

}  // namespace xpano::gui
