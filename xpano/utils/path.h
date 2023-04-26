#pragma once

#include <filesystem>
#include <vector>

namespace xpano::utils::path {

bool IsExtensionSupported(const std::filesystem::path& path);

std::vector<std::filesystem::path> KeepSupported(
    const std::vector<std::filesystem::path>& paths);

}  // namespace xpano::utils::path
