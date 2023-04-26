#pragma once

#include <optional>
#include <utility>

#include "xpano/cli/args.h"

namespace xpano::cli {

enum class ResultType {
  kSuccess,
  kError,
  kForwardToGui,
};

std::pair<ResultType, std::optional<Args>> Run(int argc, char** argv);

int ExitCode(ResultType result);

}  // namespace xpano::cli
