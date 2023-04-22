#include "xpano/cli/args.h"

#include <filesystem>
#include <vector>

#include <spdlog/spdlog.h>

#include "xpano/utils/path.h"

namespace xpano::cli {

namespace {
std::optional<std::filesystem::path> ParsePath(const std::string& arg) {
  try {
    auto path = std::filesystem::path(arg);
    if (path.is_relative()) {
      path = std::filesystem::current_path() / path;
    }
    return path;
  } catch (const std::filesystem::filesystem_error& e) {
    spdlog::error("Invalid path: {}", arg);
  }
  return {};
}

bool ValidateArgs(const Args& args) {
  if (args.output_path && args.input_paths.empty()) {
    spdlog::error("No supported images provided");
    return false;
  }
  if (args.output_path &&
      !utils::path::IsExtensionSupported(*args.output_path)) {
    spdlog::error("Unsupported output file extension: \"{}\"",
                  args.output_path->extension().string());
    return false;
  }
  return true;
}

}  // namespace

std::optional<Args> ParseArgs(int argc, char** argv) {
  Args result;
  std::vector<std::filesystem::path> input_paths;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--gui") {
      result.run_gui = true;
    } else if (arg.starts_with("--output=")) {
      if (auto path = ParsePath(arg.substr(9)); path) {
        result.output_path = *path;
      } else {
        return {};
      }
    } else {
      if (auto path = ParsePath(arg); path) {
        input_paths.push_back(*path);
      } else {
        return {};
      }
    }
  }
  result.input_paths = utils::path::KeepSupported(input_paths);

  if (!ValidateArgs(result)) {
    return {};
  }

  return result;
}

std::string Help() {
  return "Usage: xpano [--gui] [--output=<path>] [<input files>]";
}

}  // namespace xpano::cli
