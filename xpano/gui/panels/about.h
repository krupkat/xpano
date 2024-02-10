// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Vaibhav Sharma
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <future>
#include <optional>
#include <string>

#include "xpano/utils/text.h"

namespace xpano::gui {

class AboutPane {
 public:
  explicit AboutPane(std::future<utils::Texts> licenses);
  void Draw();
  void Show();

  std::optional<utils::Text> GetText(const std::string& name);

 private:
  void WaitForLicenseLoading();

  bool show_ = false;
  int current_license_ = 0;
  std::future<utils::Texts> licenses_future_;
  utils::Texts licenses_;
};

}  // namespace xpano::gui
