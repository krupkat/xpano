// SPDX-FileCopyrightText: 2024 Tomas Krupka

#pragma once

// IWYU pragma: begin_exports

#ifndef SPDLOG_FMT_EXTERNAL

#include <spdlog/fmt/fmt.h>

#else

#include <fmt/core.h>
#include <fmt/format.h>

#if FMT_VERSION >= 110000
#include <fmt/ranges.h>  // fmt::join moved here in fmt 11.0.0
#endif

#endif

// IWYU pragma: end_exports