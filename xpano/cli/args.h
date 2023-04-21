#pragma once

#include <filesystem>
#include <vector>

namespace xpano::cli {
struct Args {
  bool run_gui = false;
  std::vector<std::filesystem::path> input_paths;
};

Args ParseArgs(int argc, char** argv);

}  // namespace xpano::cli
