#pragma once

#include <string>
#include <tuple>

namespace xpano::version {

constexpr int kMajor = 0;
constexpr int kMinor = 11;
constexpr int kPatch = 0;

using Triplet = std::tuple<int, int, int>;

constexpr Triplet Current() { return {kMajor, kMinor, kPatch}; }

inline std::string ToString(Triplet version) {
  return std::to_string(std::get<0>(version)) + "." +
         std::to_string(std::get<1>(version)) + "." +
         std::to_string(std::get<2>(version));
}

}  // namespace xpano::version
