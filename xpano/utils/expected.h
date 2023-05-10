// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <tl/expected.hpp>

namespace xpano::utils {

// TODO(krupkat): switch to std::expected from C++23

template <typename TType, typename TError>
using Expected = tl::expected<TType, TError>;

template <typename TError>
using Unexpected = tl::unexpected<TError>;

}  // namespace xpano::utils
