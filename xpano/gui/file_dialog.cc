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

namespace xpano::gui::file_dialog {

namespace {

std::vector<std::filesystem::path> MultifileOpen() {
  NFD::UniquePathSet out_paths;

  std::array<nfdfilteritem_t, 1> filter_item;
  auto extensions = fmt::format("{}", fmt::join(kSupportedExtensions, ","));
  filter_item[0] = {"Images", extensions.c_str()};
  std::vector<std::filesystem::path> results;

  // show the dialog
  nfdresult_t result =
      NFD::OpenDialogMultiple(out_paths, filter_item.data(), 1);
  if (result == NFD_OKAY) {
    spdlog::info("Selected files [OpenDialogMultiple]");

    nfdpathsetsize_t num_paths;
    NFD::PathSet::Count(out_paths, num_paths);

    for (nfdpathsetsize_t i = 0; i < num_paths; ++i) {
      NFD::UniquePathSetPath path;
      NFD::PathSet::GetPath(out_paths, i, path);
      results.emplace_back(path.get());
    }
  } else if (result == NFD_CANCEL) {
    spdlog::info("User pressed cancel.");
  } else {
    spdlog::error("Error: %s", NFD::GetError());
  }

  return results;
}

std::vector<std::filesystem::path> DirectoryOpen() {
  NFD::UniquePath out_path;
  std::vector<std::filesystem::path> results;
  nfdresult_t result = NFD::PickFolder(out_path);
  if (result == NFD_OKAY) {
    spdlog::info("Selected directory {}", out_path.get());
    for (const auto& file :
         std::filesystem::directory_iterator(out_path.get())) {
      results.emplace_back(file.path());
    }
  } else if (result == NFD_CANCEL) {
    spdlog::info("User pressed cancel.");
  } else {
    spdlog::error("Error: %s", NFD::GetError());
  }
  std::sort(results.begin(), results.end());
  return results;
}

std::string LowercaseExtension(const std::filesystem::path& path) {
  auto extension = path.extension().string().substr(1);
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char letter) { return std::tolower(letter); });
  return extension;
}

bool IsExtensionSupported(const std::filesystem::path& path) {
  return path.has_extension() &&
         std::find(kSupportedExtensions.begin(), kSupportedExtensions.end(),
                   LowercaseExtension(path)) != kSupportedExtensions.end();
}

}  // namespace

std::vector<std::string> Open(Action action) {
  std::vector<std::filesystem::path> paths;

  if (action.type == ActionType::kOpenFiles) {
    paths = MultifileOpen();
  }

  if (action.type == ActionType::kOpenDirectory) {
    paths = DirectoryOpen();
  }

  std::vector<std::filesystem::path> valid_paths;
  std::copy_if(paths.begin(), paths.end(), std::back_inserter(valid_paths),
               IsExtensionSupported);

  std::vector<std::string> results;
  std::transform(
      valid_paths.begin(), valid_paths.end(), std::back_inserter(results),
      [](const std::filesystem::path& path) { return path.string(); });
  return results;
}

std::optional<std::string> Save(const std::string& default_name) {
  NFD::UniquePath out_path;
  std::array<nfdfilteritem_t, 1> filter_item;
  auto extensions = fmt::format("{}", fmt::join(kSupportedExtensions, ","));
  filter_item[0] = {"Images", extensions.c_str()};

  nfdresult_t result = NFD::SaveDialog(out_path, filter_item.data(), 1, nullptr,
                                       default_name.c_str());
  if (result == NFD_OKAY) {
    spdlog::info("Picked save file {}", out_path.get());
    if (IsExtensionSupported(out_path.get())) {
      return out_path.get();
    }
    spdlog::error("Unsupported extension");
  } else if (result == NFD_CANCEL) {
    spdlog::info("User pressed cancel.");
  } else {
    spdlog::error("Error: %s", NFD::GetError());
  }
  return {};
}

}  // namespace xpano::gui::file_dialog
