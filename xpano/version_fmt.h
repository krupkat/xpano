// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include <spdlog/fmt/fmt.h>

#include "xpano/version.h"

template <>
struct fmt::formatter<xpano::version::Triplet> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const xpano::version::Triplet& version, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "{}.{}.{}", std::get<0>(version),
                          std::get<1>(version), std::get<2>(version));
  }
};
