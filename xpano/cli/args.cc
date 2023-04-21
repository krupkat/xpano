#include "xpano/cli/args.h"

#include <filesystem>
#include <vector>

#include "xpano/utils/path.h"

namespace xpano::cli {

namespace {
std::filesystem::path ParsePath(const std::string& arg) {
  auto path = std::filesystem::path(arg);

  if (path.is_relative()) {
    path = std::filesystem::current_path() / path;
  }

  return path;
}
}  // namespace

Args ParseArgs(int argc, char** argv) {
  Args result;
  std::vector<std::filesystem::path> paths;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--gui") {
      result.run_gui = true;
    } else {
      paths.push_back(ParsePath(arg));
    }
  }

  result.input_paths = utils::path::KeepSupported(paths);
  return result;
}

}  // namespace xpano::cli
