// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Vaibhav Sharma
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace xpano::utils {

struct Text {
  std::string name;
  std::vector<std::string> lines;
};

std::vector<Text> LoadTexts(const std::filesystem::path& executable_path,
                            const std::string& rel_path);

using Texts = std::vector<Text>;

}  // namespace xpano::utils
