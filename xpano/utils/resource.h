// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <SDL.h>

namespace xpano::utils::resource {

std::optional<std::string> Find(const std::filesystem::path& executable_path,
                                const std::string& rel_path);

using SdlSurface = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;

SdlSurface LoadIcon(const std::filesystem::path& executable_path,
                    const std::string& rel_path);

}  // namespace xpano::utils::resource
