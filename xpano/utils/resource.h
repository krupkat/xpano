// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>

namespace xpano::utils::resource {

std::optional<std::string> Find(const std::filesystem::path& executable_path,
                                std::string_view rel_path);

using SdlSurface = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;

SdlSurface LoadIcon(const std::filesystem::path& executable_path,
                    std::string_view rel_path);

}  // namespace xpano::utils::resource
