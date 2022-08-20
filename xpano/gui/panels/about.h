#pragma once

#include <future>
#include <vector>

#include "utils/text.h"
using Texts = std::vector<xpano::utils::Text>;

namespace xpano::gui {

class AboutPane {
 public:
  explicit AboutPane(std::future<Texts> licenses);
  void Draw();
  void Show();

 private:
  bool show_ = false;
  int current_license_ = 0;
  std::future<Texts> licenses_future_;
  Texts licenses_;
};

}  // namespace xpano::gui
