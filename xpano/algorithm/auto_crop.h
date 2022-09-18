#pragma once

#include <optional>

#include <opencv2/core.hpp>

#include "xpano/utils/rect.h"

namespace xpano::algorithm::crop {

std::optional<utils::RectPPi> FindLargestCrop(cv::Mat mask);

}
