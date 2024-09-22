// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <optional>
#include <utility>

#include "xpano/cli/args.h"

namespace xpano::cli {

enum class ResultType : std::uint8_t {
  kSuccess,
  kError,
  kForwardToGui,
};

std::pair<ResultType, std::optional<Args>> Run(int argc, char** argv);

int ExitCode(ResultType result);

}  // namespace xpano::cli
