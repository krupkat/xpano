// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace xpano::cli {

struct Args {
  bool run_gui = false;
  bool print_help = false;
  bool print_version = false;
  std::vector<std::filesystem::path> input_paths;
  std::optional<std::filesystem::path> output_path;
};

std::optional<Args> ParseArgs(int argc, char** argv);

void PrintHelp();

}  // namespace xpano::cli
