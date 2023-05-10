// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/algorithm/image.h"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <spdlog/spdlog.h>

#include "xpano/constants.h"

namespace xpano::algorithm {
namespace {
thread_local cv::Ptr<cv::Feature2D> sift = cv::SIFT::create(kNumFeatures);

std::optional<cv::Size> PreviewSize(const cv::Size& full_size,
                                    int preview_longer_side) {
  if (std::max(full_size.width, full_size.height) <= preview_longer_side) {
    return {};
  }

  return (full_size.width > full_size.height)
             ? cv::Size(preview_longer_side,
                        static_cast<int>(preview_longer_side /
                                         full_size.aspectRatio()))
             : cv::Size(static_cast<int>(preview_longer_side *
                                         full_size.aspectRatio()),
                        preview_longer_side);
}

}  // namespace

Image::Image(std::filesystem::path path) : path_(std::move(path)) {}

void Image::Load(ImageLoadOptions options) {
  cv::Mat tmp =
      cv::imread(path_.string(), cv::IMREAD_COLOR | cv::IMREAD_ANYDEPTH);
  if (!tmp.empty() && tmp.depth() != CV_8U) {
    is_raw_ = true;
    spdlog::warn("Image {} is not 8-bit, converting", path_.string());
    tmp = cv::imread(path_.string());
  }
  if (tmp.empty()) {
    spdlog::error("Failed to load image {}", path_.string());
    return;
  }

  if (auto preview_size = PreviewSize(tmp.size(), options.preview_longer_side);
      preview_size) {
    cv::resize(tmp, preview_, *preview_size, 0.0, 0.0, cv::INTER_AREA);
  } else {
    preview_ = tmp;
  }

  if (options.compute_keypoints) {
    sift->detectAndCompute(preview_, cv::Mat(), keypoints_, descriptors_);
  }
  cv::resize(preview_, thumbnail_, cv::Size(kThumbnailSize, kThumbnailSize), 0,
             0, cv::INTER_AREA);

  spdlog::info("Loaded {}", path_.string());
  if (options.compute_keypoints) {
    spdlog::info("Size: {} x {}, Keypoints: {}", preview_.size[1],
                 preview_.size[0], keypoints_.size());
  } else {
    spdlog::info("Size: {} x {}", preview_.size[1], preview_.size[0]);
  }
}

bool Image::IsLoaded() const { return !preview_.empty(); }

bool Image::IsRaw() const { return is_raw_; }

cv::Mat Image::GetFullRes() const { return cv::imread(path_.string()); }
cv::Mat Image::GetThumbnail() const { return thumbnail_; }
cv::Mat Image::GetPreview() const { return preview_; }

float Image::GetAspect() const {
  auto width = static_cast<float>(preview_.size[1]);
  auto height = static_cast<float>(preview_.size[0]);
  return width / height;
}

cv::Mat Image::Draw(bool show_debug) const {
  if (show_debug) {
    cv::Mat tmp;
    cv::drawKeypoints(preview_, keypoints_, tmp, cv::Scalar::all(-1),
                      cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    return tmp;
  }
  return preview_;
}

const std::vector<cv::KeyPoint>& Image::GetKeypoints() const {
  return keypoints_;
}

cv::Mat Image::GetDescriptors() const { return descriptors_; }

std::filesystem::path Image::GetPath() const { return path_; }

std::string Image::PanoName() const {
  return path_.stem().string() + kDefaultPanoSuffix +
         path_.extension().string();
}

}  // namespace xpano::algorithm
