// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>

#include <opencv2/core.hpp>

#include "xpano/utils/rect.h"

namespace xpano::algorithm::crop {

constexpr unsigned char kMaskValueOn = 0xFF;

std::optional<utils::RectPPi> FindLargestCrop(const cv::Mat& mask);

}  // namespace xpano::algorithm::crop
