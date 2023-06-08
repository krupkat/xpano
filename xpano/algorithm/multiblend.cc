// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/algorithm/multiblend.h"

#include <cstring>
#include <stdexcept>

#ifdef XPANO_WITH_MULTIBLEND
#include <mb/flex.h>
#include <mb/multiblend.h>
#include <mb/threadpool.h>
#endif
#include <spdlog/fmt/fmt.h>

namespace xpano::algorithm::mb {

namespace {
constexpr int kChannelDepth = 8;
constexpr uint32_t kFlagBit = 0x80000000u;
constexpr uint32_t kWithoutFlag = 0x7fffffffu;
constexpr uint8_t kMaskOn = 0xffu;
constexpr uint8_t kMaskOff = 0x00u;

void SafeMemset(uint8_t *ptr, uint8_t value, size_t num, const uint8_t *end) {
  if (num == 0) {
    throw(std::runtime_error("Multiblend: invalid mask format"));
  }
  if (ptr + num > end) {
    throw(std::runtime_error("Multiblend: mask out of bounds"));
  }
  std::memset(ptr, value, num);
}

// Convert from Multiblend's Flex format to OpenCV's UMat.
// Flex is a RLE format, leftmost bit is the mask flag, the rest is the length.
cv::UMat ToUMat(multiblend::utils::Flex &flex) {
  auto mask = cv::Mat(flex.height_, flex.width_, CV_8U);

  flex.Start();

  for (int y = 0; y < mask.rows; y++) {
    auto *ptr = mask.ptr<uint8_t>(y);
    auto *end = ptr + mask.cols;

    while (ptr < end) {
      auto length_with_flag = flex.SafeReadForwards32();
      if ((length_with_flag & kFlagBit) != 0u) {
        auto length = length_with_flag & kWithoutFlag;
        SafeMemset(ptr, kMaskOn, length, end);
        ptr += length;
      } else {
        SafeMemset(ptr, kMaskOff, length_with_flag, end);
        ptr += length_with_flag;
      }
    }
  }

  return mask.getUMat(cv::ACCESS_READ);
}

template <typename TChannelType>
cv::UMat Merge(const std::array<TChannelType, 3> &mb_channels, int width,
               int height) {
  auto blue = cv::Mat(height, width, CV_8UC1, mb_channels[0].get());
  auto green = cv::Mat(height, width, CV_8UC1, mb_channels[1].get());
  auto red = cv::Mat(height, width, CV_8UC1, mb_channels[2].get());

  std::vector<cv::UMat> channels{blue.getUMat(cv::ACCESS_READ),
                                 green.getUMat(cv::ACCESS_READ),
                                 red.getUMat(cv::ACCESS_READ)};
  cv::UMat pano;
  cv::merge(channels, pano);
  return pano;
}

std::vector<uint8_t> ToVector(const cv::Mat &img) {
  CV_Assert(img.type() == CV_8UC4 || img.type() == CV_8UC3);

  std::vector<uint8_t> result(img.total() * img.elemSize());

  if (img.isContinuous()) {
    std::memcpy(result.data(), img.data, result.size());
    return result;
  }

  uint8_t *result_data = result.data();
  auto row_size = img.cols * img.elemSize();
  for (int y = 0; y < img.rows; ++y, result_data += row_size) {
    const auto *src_row = img.ptr<uint8_t>(y);
    std::memcpy(result_data, src_row, row_size);
  }
  return result;
}

cv::UMat Merge(cv::InputArray input_img, cv::InputArray input_mask) {
  cv::UMat input_img_8bit;
  input_img.getUMat().convertTo(input_img_8bit, CV_8UC3);

  std::vector<cv::UMat> channels;
  cv::split(input_img_8bit, channels);
  input_img_8bit.release();

  cv::UMat mask;
  // Multiblend only works with the mask as binary, this conversion prevents
  // artifacts where the mask is not 0 or 255.
  cv::compare(input_mask, 0, mask, cv::CMP_NE);
  channels.push_back(mask);

  cv::UMat input_with_alpha;
  cv::merge(channels, input_with_alpha);
  return input_with_alpha;
}

}  // namespace

void MultiblendBlender::prepare(cv::Rect dst_roi) {
  if (blending_method_ == BlendingMethod::kMultiblend) {
    dst_mask_.create(dst_roi.size(), CV_8U);
    dst_mask_.setTo(cv::Scalar::all(0));
  }
  dst_roi_ = dst_roi;
}

// Note: better work with UMat whenever possible to get a speedup from OpenCL,
// the inputs and outputs are already expected to be UMats in the Stitcher code.
void MultiblendBlender::feed(cv::InputArray input_img,
                             cv::InputArray input_mask, cv::Point top_left) {
#ifdef XPANO_WITH_MULTIBLEND
  CV_Assert(input_img.type() == CV_16SC3);
  CV_Assert(input_mask.type() == CV_8U);

  cv::UMat input_umat;

  if (blending_method_ == BlendingMethod::kMultiblendAlpha) {
    input_umat = Merge(input_img, input_mask);
  } else if (blending_method_ == BlendingMethod::kMultiblend) {
    input_img.getUMat().convertTo(input_umat, CV_8UC3);
    feed_mask(input_mask, top_left);
  } else {
    throw(std::runtime_error("Multiblend: Invalid blending method"));
  }

  images_.emplace_back(multiblend::io::InMemoryImage{
      .tiff_width = input_umat.cols,
      .tiff_height = input_umat.rows,
      .bpp = kChannelDepth,
      .spp = static_cast<uint16_t>(input_umat.channels()),
      .xpos_add = top_left.x,
      .ypos_add = top_left.y,
      .data = ToVector(input_umat.getMat(cv::ACCESS_READ))});
#else
  throw(std::runtime_error("Multiblend support not compiled in"));
#endif
}

void MultiblendBlender::feed_mask(cv::InputArray mask, cv::Point top_left) {
  int start_x = top_left.x - dst_roi_.x;
  int start_y = top_left.y - dst_roi_.y;

  cv::Mat src_mask = mask.getMat();
  cv::Mat dst_mask = dst_mask_.getMat(cv::ACCESS_RW);

  for (int y = 0; y < src_mask.rows; ++y) {
    if (start_y + y < 0 || start_y + y >= dst_mask.rows) {
      throw(std::runtime_error("Multiblend: Invalid mask size"));
    }

    const auto *src_mask_row = src_mask.ptr<uint8_t>(y);
    auto *dst_mask_row = dst_mask.ptr<uint8_t>(start_y + y);

    if (start_x < 0 || start_x + src_mask.cols > dst_mask.cols) {
      throw(std::runtime_error("Multiblend: Invalid mask size"));
    }

    for (int x = 0; x < src_mask.cols; ++x) {
      dst_mask_row[start_x + x] |= src_mask_row[x];
    }
  }
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

  if (blending_method_ == BlendingMethod::kMultiblend &&
      (result.width != dst_mask_.cols || result.height != dst_mask_.rows)) {
    throw(std::runtime_error(fmt::format(
        "Multiblend: Returned an image of size {}x{}, expected {}x{}",
        result.width, result.height, dst_mask_.cols, dst_mask_.rows)));
  }

  auto pano = Merge(result.output_channels, result.width, result.height);

  if (blending_method_ == BlendingMethod::kMultiblendAlpha) {
    dst_mask_ = ToUMat(result.full_mask);
  }

  cv::UMat zeroes;
  cv::compare(dst_mask_, 0, zeroes, cv::CMP_EQ);
  pano.setTo(cv::Scalar::all(0), zeroes);

  dst.assign(pano);
  dst_mask.assign(dst_mask_);
  dst_mask_.release();
#else
  throw(std::runtime_error("Multiblend support not compiled in"));
#endif
}

}  // namespace xpano::algorithm::mb
