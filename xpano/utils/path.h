// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <vector>

namespace xpano::utils::path {

bool IsExtensionSupported(const std::filesystem::path& path);

bool IsMetadataExtensionSupported(const std::filesystem::path& path);

enum class ImageType : std::uint8_t { kJPG, kPNG, kOther };

ImageType GetImageType(const std::filesystem::path& path);

std::vector<std::filesystem::path> KeepSupported(
    const std::vector<std::filesystem::path>& paths);

}  // namespace xpano::utils::path
