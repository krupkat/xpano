#pragma once

#include <string>
#include <tuple>

namespace xpano::version {

constexpr int kMajor = 0;
constexpr int kMinor = 11;
constexpr int kPatch = 0;

using Triplet = std::tuple<int, int, int>;

constexpr Triplet Current() { return {kMajor, kMinor, kPatch}; }

}  // namespace xpano::version
