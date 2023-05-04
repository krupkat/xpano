// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/log/logger.h"

namespace xpano::gui {

class LogPane {
 public:
  explicit LogPane(logger::Logger* logger);
  void Draw();
  void ToggleShow();
  [[nodiscard]] bool IsShown() const;

 private:
  logger::Logger* logger_;
  bool show_ = false;
};

}  // namespace xpano::gui
