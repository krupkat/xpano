// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "xpano/gui/action.h"
#include "xpano/utils/expected.h"
#include "xpano/utils/fmt.h"

namespace xpano::gui::file_dialog {

enum class ErrorType : std::uint8_t {
  kUserCancelled,
  kTargetNotDirectory,
  kUnsupportedExtension,
  kUnknownAction,
  kUnknownError
};

struct Error {
  ErrorType type;
  std::string message;
};

utils::Expected<std::vector<std::filesystem::path>, Error> Open(
    const Action& action);

utils::Expected<std::filesystem::path, Error> Save(
    const std::string& default_name);

}  // namespace xpano::gui::file_dialog

template <>
struct fmt::formatter<xpano::gui::file_dialog::Error> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const xpano::gui::file_dialog::Error& error,
              FormatContext& ctx) const -> decltype(ctx.out()) {
    using ErrorType = xpano::gui::file_dialog::ErrorType;

    switch (error.type) {
      case ErrorType::kUserCancelled:
        return fmt::format_to(ctx.out(), "User cancelled");
      case ErrorType::kTargetNotDirectory:
        return fmt::format_to(ctx.out(), "Target \"{}\" is not a directory",
                              error.message);
      case ErrorType::kUnsupportedExtension:
        return fmt::format_to(ctx.out(), "Unsupported extension \"{}\"",
                              error.message);
      case ErrorType::kUnknownAction:
        return fmt::format_to(ctx.out(), "Unknown action");
      case ErrorType::kUnknownError:
        return fmt::format_to(ctx.out(), "Unknown error: \"{}\"",
                              error.message);
    }

    return ctx.out();
  }
};
