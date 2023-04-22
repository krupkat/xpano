#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace xpano::cli {

struct Args {
  bool run_gui = false;
  std::vector<std::filesystem::path> input_paths;
  std::optional<std::filesystem::path> output_path;
};

std::optional<Args> ParseArgs(int argc, char** argv);

std::string Help();

}  // namespace xpano::cli
