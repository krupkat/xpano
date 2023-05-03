#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <spdlog/fmt/fmt.h>

#include "xpano/gui/action.h"
#include "xpano/utils/expected.h"

namespace xpano::gui::file_dialog {

enum class ErrorType {
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
