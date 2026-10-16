// SPDX-FileCopyrightText: 2026 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <expected>
#include <filesystem>
#include <future>
#include <string>
#include <variant>
#include <vector>

#include "xpano/gui/action.h"
#include "xpano/gui/file_dialog/error.h"

namespace xpano::gui::file_dialog {

using OpenResult = std::expected<std::vector<std::filesystem::path>, Error>;
using SaveResult = std::expected<std::filesystem::path, Error>;
using Future = std::variant<std::future<OpenResult>, std::future<SaveResult>>;

class Base {
 public:
  virtual ~Base() = default;
  virtual std::future<OpenResult> Open(const Action& action) = 0;
  virtual std::future<SaveResult> Save(const std::string& default_name) = 0;
};

}  // namespace xpano::gui::file_dialog
