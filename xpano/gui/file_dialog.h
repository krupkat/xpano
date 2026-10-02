// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <future>
#include <string>
#include <vector>

#include "xpano/gui/action.h"
#include "xpano/utils/fmt.h"

struct SDL_Window;

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

// Launch on the main thread and poll the returned future while pumping events.
std::future<std::expected<std::vector<std::filesystem::path>, Error>> Open(
    const Action& action, SDL_Window* window);

std::future<std::expected<std::filesystem::path, Error>> Save(
    const std::string& default_name, SDL_Window* window);

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
