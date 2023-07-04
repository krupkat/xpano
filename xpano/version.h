// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <tuple>

namespace xpano::version {

constexpr int kMajor = 0;
constexpr int kMinor = 16;
constexpr int kPatch = 0;

using Triplet = std::tuple<int, int, int>;

constexpr Triplet Current() { return {kMajor, kMinor, kPatch}; }

}  // namespace xpano::version
