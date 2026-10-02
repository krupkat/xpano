// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/file_dialog.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <expected>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <SDL3/SDL_dialog.h>
#include <spdlog/spdlog.h>

#include "xpano/constants.h"
#include "xpano/gui/action.h"
#include "xpano/utils/fmt.h"
#include "xpano/utils/path.h"

namespace xpano::gui::file_dialog {

namespace {

template <typename... Args>
std::unexpected<Error> MakeUnexpected(Args&&... args) {
  return std::unexpected<Error>(Error{std::forward<Args>(args)...});
}

std::string Extensions() {
  auto extensions = fmt::format("{}", fmt::join(kSupportedExtensions, ";"));
#ifndef _WIN32
  auto uppercase = extensions;
  std::ranges::transform(
      uppercase, uppercase.begin(),
      [](unsigned char letter) { return std::toupper(letter); });
  extensions += ";" + uppercase;
#endif
  return extensions;
}

using Paths = std::vector<std::filesystem::path>;

// SDL may call back on another thread, even after the GUI has been destroyed.
// The callback owns this state; it never accesses GUI objects. Filters and the
// default location must also outlive the asynchronous dialog.
template <typename T>
struct DialogState {
  std::promise<std::expected<T, Error>> promise;
  std::function<std::expected<T, Error>(Paths)> process;
  std::string extensions = Extensions();
  SDL_DialogFileFilter filter{"Images", extensions.c_str()};
  std::string default_location;
};

template <typename T>
void SDLCALL DialogCallback(void* userdata, const char* const* files, int) {
  std::unique_ptr<DialogState<T>> state(static_cast<DialogState<T>*>(userdata));
  try {
    if (files == nullptr) {
      state->promise.set_value(
          MakeUnexpected(ErrorType::kUnknownError, SDL_GetError()));
    } else if (*files == nullptr) {
      state->promise.set_value(MakeUnexpected(ErrorType::kUserCancelled));
    } else {
      Paths paths;
      for (; *files != nullptr; ++files) {
        // SDL returns UTF-8 on every platform, including Windows.
        const std::string_view path(*files);
        paths.emplace_back(std::u8string(path.begin(), path.end()));
      }
      state->promise.set_value(state->process(std::move(paths)));
    }
  } catch (const std::exception& error) {
    state->promise.set_value(
        MakeUnexpected(ErrorType::kUnknownError, error.what()));
  }
}

std::expected<Paths, Error> DirectoryOpen(Paths paths) {
  const auto& dir_path = paths.front();
  if (!std::filesystem::is_directory(dir_path)) {
    return MakeUnexpected(ErrorType::kTargetNotDirectory, dir_path.string());
  }
  spdlog::info("Selected directory {}", dir_path.string());

  Paths results;
  for (const auto& file : std::filesystem::directory_iterator(dir_path)) {
    results.emplace_back(file.path());
  }
  std::ranges::sort(results);
  return utils::path::KeepSupported(results);
}

}  // namespace

std::future<std::expected<Paths, Error>> Open(const Action& action,
                                              SDL_Window* window) {
  auto state = std::make_unique<DialogState<Paths>>();
  auto future = state->promise.get_future();
  if (action.type == ActionType::kOpenFiles) {
    state->process = [](Paths paths) -> std::expected<Paths, Error> {
      spdlog::info("Selected files [SDL_ShowOpenFileDialog]");
      return utils::path::KeepSupported(paths);
    };
    auto* data = state.release();
    SDL_ShowOpenFileDialog(DialogCallback<Paths>, data, window, &data->filter,
                           1, nullptr, true);
  } else if (action.type == ActionType::kOpenDirectory) {
    state->process = DirectoryOpen;
    SDL_ShowOpenFolderDialog(DialogCallback<Paths>, state.release(), window,
                             nullptr, false);
  } else {
    state->promise.set_value(MakeUnexpected(ErrorType::kUnknownAction));
  }
  return future;
}

std::future<std::expected<std::filesystem::path, Error>> Save(
    const std::string& default_name, SDL_Window* window) {
  auto state = std::make_unique<DialogState<std::filesystem::path>>();
  auto future = state->promise.get_future();
  state->default_location = default_name;
  state->process =
      [](Paths paths) -> std::expected<std::filesystem::path, Error> {
    auto result_path = std::move(paths.front());
    spdlog::info("Picked save file {}", result_path.string());
    if (!utils::path::IsExtensionSupported(result_path)) {
      return MakeUnexpected(ErrorType::kUnsupportedExtension,
                            result_path.filename().string());
    }
    return result_path;
  };
  auto* data = state.release();
  SDL_ShowSaveFileDialog(DialogCallback<std::filesystem::path>, data, window,
                         &data->filter, 1, data->default_location.c_str());
  return future;
}

}  // namespace xpano::gui::file_dialog
