// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>

#include "xpano/utils/vec.h"

namespace xpano::utils::exiv2 {

constexpr bool Enabled() {
#ifdef XPANO_WITH_EXIV2
  return true;
#else
  return false;
#endif
}

void CreateExif(const std::optional<std::filesystem::path>& from_path,
                const std::filesystem::path& to_path, const Vec2i& image_size);

}  // namespace xpano::utils::exiv2
