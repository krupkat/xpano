// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/cli/args.h"

#include <exception>
#include <filesystem>
#include <string>
#include <vector>

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "xpano/constants.h"
#include "xpano/utils/path.h"

namespace xpano::cli {

namespace {

// Custom args parsing...
// TODO(krupkat): move to cxxopts / cli11 when adding new arguments

const std::string kGuiFlag = "--gui";
const std::string kOutputFlag = "--output=";
const std::string kHelpFlag = "--help";
const std::string kVersionFlag = "--version";

void ParseArg(Args* result, const std::string& arg) {
  if (arg == kGuiFlag) {
    result->run_gui = true;
  } else if (arg == kHelpFlag) {
    result->print_help = true;
  } else if (arg == kVersionFlag) {
    result->print_version = true;
  } else if (arg.starts_with(kOutputFlag)) {
    auto substr = arg.substr(kOutputFlag.size());
    result->output_path = std::filesystem::path(substr);
  } else {
    result->input_paths.emplace_back(arg);
  }
}

Args ParseArgsRaw(int argc, char** argv) {
  Args result;
  const std::vector<std::filesystem::path> input_paths;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    ParseArg(&result, arg);
  }
  return result;
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
  if (args.output_path && args.run_gui) {
    spdlog::error(
        "Specifying --gui and --output together is not yet supported.");
    return false;
  }
  return true;
}

}  // namespace

std::optional<Args> ParseArgs(int argc, char** argv) {
  Args args;
  try {
    args = ParseArgsRaw(argc, argv);
  } catch (const std::exception& e) {
    spdlog::error("Error parsing arguments: {}", e.what());
    return {};
  }

  auto supported_inputs = utils::path::KeepSupported(args.input_paths);
  if (supported_inputs.empty() && !args.input_paths.empty()) {
    spdlog::error("No supported images provided!");
    return {};
  }
  args.input_paths = supported_inputs;

  if (!ValidateArgs(args)) {
    return {};
  }

  return args;
}

void PrintHelp() {
  spdlog::info("Usage: Xpano [<input files>] [--output=<path>]");
  spdlog::info("\t[--gui] [--help] [--version]");
  spdlog::info("Supported formats: {}", fmt::join(kSupportedExtensions, ", "));
}

}  // namespace xpano::cli
