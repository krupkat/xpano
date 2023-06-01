// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>

#ifdef XPANO_WITH_MULTIBLEND
#include <mb/image.h>
#endif
#include <opencv2/stitching/detail/blenders.hpp>

#include "xpano/utils/threadpool.h"

namespace xpano::algorithm::mb {

constexpr bool Enabled() {
#ifdef XPANO_WITH_MULTIBLEND
  return true;
#else
  return false;
#endif
}

class MultiblendBlender : public cv::detail::Blender {
 public:
  explicit MultiblendBlender(utils::mt::Threadpool* threadpool)
      : threadpool_(threadpool) {}
  void prepare(cv::Rect dst_roi) override;
  void feed(cv::InputArray img, cv::InputArray mask,
            cv::Point top_left) override;
  void blend(cv::InputOutputArray dst, cv::InputOutputArray dst_mask) override;

 private:
#ifdef XPANO_WITH_MULTIBLEND
  std::vector<multiblend::io::Image> images_;
#endif
  utils::mt::Threadpool* threadpool_;
};

}  // namespace xpano::algorithm::mb
