#pragma once

#include <future>
#include <vector>

#include "utils/text.h"

namespace xpano::gui {

class AboutPane {
 public:
  explicit AboutPane(std::future<utils::Texts> licenses);
  void Draw();
  void Show();

 private:
  bool show_ = false;
  int current_license_ = 0;
  std::future<utils::Texts> licenses_future_;
  utils::Texts licenses_;
};

}  // namespace xpano::gui
