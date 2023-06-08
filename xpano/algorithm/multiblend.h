// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>

#ifdef XPANO_WITH_MULTIBLEND
#include <mb/image.h>
#endif
#include <opencv2/stitching/detail/blenders.hpp>

#include "xpano/algorithm/options.h"
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
  explicit MultiblendBlender(utils::mt::Threadpool* threadpool,
                             BlendingMethod blending_method)
      : threadpool_(threadpool), blending_method_(blending_method) {}
  void prepare(cv::Rect dst_roi) override;
  void feed(cv::InputArray img, cv::InputArray mask,
            cv::Point top_left) override;
  void blend(cv::InputOutputArray dst, cv::InputOutputArray dst_mask) override;

 private:
  void feed_mask(cv::InputArray mask, cv::Point top_left);

#ifdef XPANO_WITH_MULTIBLEND
  std::vector<multiblend::io::Image> images_;
#endif
  utils::mt::Threadpool* threadpool_;
  BlendingMethod blending_method_;
};

}  // namespace xpano::algorithm::mb
