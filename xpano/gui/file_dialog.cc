#include "xpano/gui/file_dialog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <iterator>
#include <string>
#include <vector>

#include <nfd.h>
#include <nfd.hpp>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "xpano/constants.h"
#include "xpano/utils/expected.h"
#include "xpano/utils/path.h"

namespace xpano::gui::file_dialog {

namespace {

template <typename TArray>
TArray Uppercase(const TArray& extensions) {
  TArray result;
  auto to_upper = [](const auto& extension) {
    auto uppercase_extension = extension;
    std::transform(extension.begin(), extension.end(),
                   uppercase_extension.begin(),
                   [](const auto& letter) { return std::toupper(letter); });
    return uppercase_extension;
  };
  std::transform(extensions.begin(), extensions.end(), result.begin(),
                 to_upper);
  return result;
}

utils::Expected<std::vector<std::filesystem::path>, Error> MultifileOpen() {
  NFD::UniquePathSet out_paths;

  auto extensions = fmt::format("{}", fmt::join(kSupportedExtensions, ","));
#ifndef _WIN32
  extensions = fmt::format("{},{}", extensions,
                           fmt::join(Uppercase(kSupportedExtensions), ","));
#endif
  auto filter_item = std::array{nfdfilteritem_t{"Images", extensions.c_str()}};
  auto nfd_result = NFD::OpenDialogMultiple(out_paths, filter_item.data(), 1);

  if (nfd_result == NFD_CANCEL) {
    return utils::Unexpected<Error>(ErrorType::kUserCancelled);
  } else if (nfd_result == NFD_ERROR) {
    return utils::Unexpected<Error>(ErrorType::kUnknownError, NFD::GetError());
  }

  spdlog::info("Selected files [OpenDialogMultiple]");

  nfdpathsetsize_t num_paths;
  NFD::PathSet::Count(out_paths, num_paths);

  std::vector<std::filesystem::path> results;
  for (nfdpathsetsize_t i = 0; i < num_paths; ++i) {
    NFD::UniquePathSetPath path;
    NFD::PathSet::GetPath(out_paths, i, path);
    results.emplace_back(path.get());
  }

  return results;
}

utils::Expected<std::vector<std::filesystem::path>, Error> DirectoryOpen() {
  NFD::UniquePath out_path;
  auto nfd_result = NFD::PickFolder(out_path);

  if (nfd_result == NFD_CANCEL) {
    return utils::Unexpected<Error>(ErrorType::kUserCancelled);
  } else if (nfd_result == NFD_ERROR) {
    return utils::Unexpected<Error>(ErrorType::kUnknownError, NFD::GetError());
  }

  auto dir_path = std::filesystem::path(out_path.get());
  if (!std::filesystem::is_directory(dir_path)) {
    return utils::Unexpected<Error>(ErrorType::kTargetNotDirectory,
                                    dir_path.string());
  }
  spdlog::info("Selected directory {}", dir_path.string());

  std::vector<std::filesystem::path> results;
  for (const auto& file : std::filesystem::directory_iterator(dir_path)) {
    results.emplace_back(file.path());
  }
  std::sort(results.begin(), results.end());
  return results;
}

}  // namespace

utils::Expected<std::vector<std::filesystem::path>, Error> Open(
    const Action& action) {
  if (action.type == ActionType::kOpenFiles) {
    return MultifileOpen().map(utils::path::KeepSupported);
  }

  if (action.type == ActionType::kOpenDirectory) {
    return DirectoryOpen().map(utils::path::KeepSupported);
  }

  return utils::Unexpected<Error>(ErrorType::kUnknownAction);
}

utils::Expected<std::filesystem::path, Error> Save(
    const std::string& default_name) {
  NFD::UniquePath out_path;
  auto extensions = fmt::format("{}", fmt::join(kSupportedExtensions, ","));
  auto filter_item = std::array{nfdfilteritem_t{"Images", extensions.c_str()}};
  auto nfd_result = NFD::SaveDialog(out_path, filter_item.data(), 1, nullptr,
                                    default_name.c_str());

  if (nfd_result == NFD_CANCEL) {
    return utils::Unexpected<Error>(ErrorType::kUserCancelled);
  } else if (nfd_result == NFD_ERROR) {
    return utils::Unexpected<Error>(ErrorType::kUnknownError, NFD::GetError());
  }

  auto result_path = std::filesystem::path(out_path.get());
  spdlog::info("Picked save file {}", result_path.string());
  if (!utils::path::IsExtensionSupported(result_path)) {
    return utils::Unexpected<Error>(ErrorType::kUnsupportedExtension,
                                    result_path.filename().string());
  }

  return result_path;
}

}  // namespace xpano::gui::file_dialog
