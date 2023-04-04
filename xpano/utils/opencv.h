#pragma once

#include <opencv2/core/version.hpp>

#define XPANO_OPENCV_HAS_JPEG_SUBSAMPLING_SUPPORT \
  (CV_VERSION_MAJOR >= 4 && CV_VERSION_MINOR >= 7)

namespace xpano::utils::opencv {

constexpr bool HasJpegSubsamplingSupport() {
  return XPANO_OPENCV_HAS_JPEG_SUBSAMPLING_SUPPORT;
}

}  // namespace xpano::utils::opencv
