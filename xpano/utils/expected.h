#pragma once

#include <variant>

#include <tl/expected.hpp>

namespace xpano::utils {

// TODO(krupkat): switch to std::expected from C++23

template <typename TType, typename TError>
using Expected = tl::expected<TType, TError>;

template <typename TError>
using Unexpected = tl::unexpected<TError>;

}  // namespace xpano::utils