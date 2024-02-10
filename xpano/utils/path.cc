// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/utils/path.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iterator>
#include <string>
#include <vector>

#include "xpano/constants.h"

namespace xpano::utils::path {

namespace {
std::string LowercaseExtension(const std::filesystem::path& path) {
  auto extension = path.extension().string().substr(1);
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char letter) { return std::tolower(letter); });
  return extension;
}

template <typename TArray>
bool ContainsExtensionIgnoreCase(const TArray& extensions,
                                 const std::filesystem::path& path) {
  return path.has_extension() &&
         std::find(extensions.begin(), extensions.end(),
                   LowercaseExtension(path)) != extensions.end();
}

}  // namespace

bool IsExtensionSupported(const std::filesystem::path& path) {
  return ContainsExtensionIgnoreCase(kSupportedExtensions, path);
}

bool IsMetadataExtensionSupported(const std::filesystem::path& path) {
  return ContainsExtensionIgnoreCase(kMetadataSupportedExtensions, path);
}

std::vector<std::filesystem::path> KeepSupported(
    const std::vector<std::filesystem::path>& paths) {
  std::vector<std::filesystem::path> valid_paths;
  std::copy_if(paths.begin(), paths.end(), std::back_inserter(valid_paths),
               IsExtensionSupported);

  return valid_paths;
}

}  // namespace xpano::utils::path
