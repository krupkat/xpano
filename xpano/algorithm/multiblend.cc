// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/algorithm/multiblend.h"

#include <cstring>
#include <stdexcept>

#ifdef XPANO_WITH_MULTIBLEND
#include <mb/multiblend.h>
#include <mb/threadpool.h>
#endif
#include <spdlog/fmt/fmt.h>

namespace xpano::algorithm::mb {

namespace {
constexpr int kChannelDepth = 8;
}

void MultiblendBlender::prepare(cv::Rect dst_roi) {
  dst_mask_.create(dst_roi.size(), CV_8U);
  dst_mask_.setTo(cv::Scalar::all(0));
  dst_roi_ = dst_roi;
}

void MultiblendBlender::feed(cv::InputArray input_img,
                             cv::InputArray input_mask, cv::Point top_left) {
#ifdef XPANO_WITH_MULTIBLEND
  CV_Assert(input_img.type() == CV_16SC3);
  CV_Assert(input_mask.type() == CV_8U);

  constexpr int kNumChannels = 3;
  auto mb_image = multiblend::io::InMemoryImage{.tiff_width = input_img.cols(),
                                                .tiff_height = input_img.rows(),
                                                .bpp = kChannelDepth,
                                                .spp = kNumChannels,
                                                .xpos_add = top_left.x,
                                                .ypos_add = top_left.y,
                                                .data = {}};

  cv::Mat mask = input_mask.getMat();
  cv::Mat dst_mask = dst_mask_.getMat(cv::ACCESS_RW);

  int start_x = top_left.x - dst_roi_.x;
  int start_y = top_left.y - dst_roi_.y;

  cv::Mat img;
  input_img.getMat().convertTo(img, CV_8UC3);
  mb_image.data.resize(img.total() * img.elemSize());

  uint8_t *mb_image_data = mb_image.data.data();
  auto row_size = img.cols * img.elemSize();

  for (int y = 0; y < img.rows; ++y, mb_image_data += row_size) {
    const auto *src_row = img.ptr<uint8_t>(y);
    std::memcpy(mb_image_data, src_row, row_size);

    const auto *mask_row = mask.ptr<uint8_t>(y);
    auto *dst_mask_row = dst_mask.ptr<uint8_t>(start_y + y);

    for (int x = 0; x < img.cols; ++x) {
      dst_mask_row[start_x + x] |= mask_row[x];
    }
  }

  images_.emplace_back(std::move(mb_image));
#else
  throw(std::runtime_error("Multiblend support not compiled in"));
#endif
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): OpenCV API
void MultiblendBlender::blend(cv::InputOutputArray dst,
                              cv::InputOutputArray dst_mask) {
#ifdef XPANO_WITH_MULTIBLEND
  auto result = multiblend::Multiblend(
      images_,
      {.output_type = multiblend::io::ImageType::MB_IN_MEMORY,
       .output_bpp = kChannelDepth},
      multiblend::mt::ThreadpoolPtr{threadpool_});

  if (result.width != dst_mask_.cols || result.height != dst_mask_.rows) {
    throw(std::runtime_error(fmt::format(
        "Multiblend returned an image of size {}x{}, expected {}x{}",
        result.width, result.height, dst_mask_.cols, dst_mask_.rows)));
  }

  auto blue = cv::Mat(result.height, result.width, CV_8UC1,
                      result.output_channels[0].get());
  auto green = cv::Mat(result.height, result.width, CV_8UC1,
                       result.output_channels[1].get());
  auto red = cv::Mat(result.height, result.width, CV_8UC1,
                     result.output_channels[2].get());

  std::vector<cv::Mat> channels{blue, green, red};

  cv::Mat pano;
  cv::merge(channels, pano);

  cv::UMat mask;
  compare(dst_mask_, 0, mask, cv::CMP_EQ);
  pano.setTo(cv::Scalar::all(0), mask);

  dst.assign(pano);
  dst_mask.assign(dst_mask_);
#else
  throw(std::runtime_error("Multiblend support not compiled in"));
#endif
}

}  // namespace xpano::algorithm::mb
