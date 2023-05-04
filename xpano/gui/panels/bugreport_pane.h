// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Naachiket Pant
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "xpano/log/logger.h"

namespace xpano::gui {

class BugReportPane {
 public:
  explicit BugReportPane(logger::Logger* logger);
  void Draw();
  void Show();

 private:
  bool show_ = false;
  logger::Logger* logger_;
};

}  // namespace xpano::gui
