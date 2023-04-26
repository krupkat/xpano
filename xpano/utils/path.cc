#include "xpano/utils/path.h"

#include <algorithm>
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
}  // namespace

bool IsExtensionSupported(const std::filesystem::path& path) {
  return path.has_extension() &&
         std::find(kSupportedExtensions.begin(), kSupportedExtensions.end(),
                   LowercaseExtension(path)) != kSupportedExtensions.end();
}

std::vector<std::filesystem::path> KeepSupported(
    const std::vector<std::filesystem::path>& paths) {
  std::vector<std::filesystem::path> valid_paths;
  std::copy_if(paths.begin(), paths.end(), std::back_inserter(valid_paths),
               IsExtensionSupported);

  return valid_paths;
}

}  // namespace xpano::utils::path
