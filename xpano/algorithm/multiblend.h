// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>

#include <mb/image.h>
#include <opencv2/stitching/detail/blenders.hpp>

namespace xpano::algorithm::mb {

class MultiblendBlender : public cv::detail::Blender {
 public:
  void prepare(cv::Rect dst_roi) override;
  void feed(cv::InputArray img, cv::InputArray mask, cv::Point tl) override;
  void blend(cv::InputOutputArray dst, cv::InputOutputArray dst_mask) override;

 private:
  std::vector<multiblend::io::Image> images_;
};

}  // namespace xpano::algorithm::mb
