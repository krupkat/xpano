#pragma once

#include "xpano/log/logger.h"

namespace xpano::gui {

class BugReportPane {
 public:
  explicit BugReportPane(logger::Logger *logger);
  void Draw();
  void Show();

 private:
  bool show_ = false;
  logger::Logger* logger_;
};

}  // namespace xpano::gui
