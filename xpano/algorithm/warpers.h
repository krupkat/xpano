// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include <opencv2/stitching/detail/warpers.hpp>
#include <opencv2/stitching/warpers.hpp>

namespace xpano::algorithm::stitcher {

class PlanePortraitWarper : public cv::WarperCreator {
 public:
  [[nodiscard]] cv::Ptr<cv::detail::RotationWarper> create(
      float scale) const override {
    return cv::makePtr<cv::detail::PlanePortraitWarper>(scale);
  }
};

class CylindricalPortraitWarper : public cv::WarperCreator {
 public:
  [[nodiscard]] cv::Ptr<cv::detail::RotationWarper> create(
      float scale) const override {
    return cv::makePtr<cv::detail::CylindricalPortraitWarper>(scale);
  }
};

class SphericalPortraitWarper : public cv::WarperCreator {
 public:
  [[nodiscard]] cv::Ptr<cv::detail::RotationWarper> create(
      float scale) const override {
    return cv::makePtr<cv::detail::SphericalPortraitWarper>(scale);
  }
};

}  // namespace xpano::algorithm::stitcher