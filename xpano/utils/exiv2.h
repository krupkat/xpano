#pragma once

#include <filesystem>

namespace xpano::utils::exiv2 {

void CreateExif(const std::filesystem::path& from_path,
                const std::filesystem::path& to_path);

}  // namespace xpano::utils::exiv2
