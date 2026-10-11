// SPDX-FileCopyrightText: 2026 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <expected>
#include <filesystem>
#include <future>
#include <string>
#include <vector>

#include "xpano/gui/action.h"
#include "xpano/gui/file_dialog/error.h"

namespace xpano::gui::file_dialog {

class Base {
 public:
  virtual ~Base() = default;

  // Launch on the main thread and poll the returned future while pumping
  // events.
  virtual std::future<
      std::expected<std::vector<std::filesystem::path>, file_dialog::Error>>
  Open(const Action& action) = 0;
  virtual std::future<std::expected<std::filesystem::path, file_dialog::Error>>
  Save(const std::string& default_name) = 0;
};

}  // namespace xpano::gui::file_dialog
