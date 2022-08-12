#include "algorithm/image.h"

#include <string>
#include <utility>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <spdlog/spdlog.h>

#include "constants.h"

namespace xpano::algorithm {
namespace {
thread_local cv::Ptr<cv::Feature2D> sift = cv::SIFT::create(kNumFeatures);
}  // namespace

Image::Image(std::string path) : path_(std::move(path)) {}

void Image::Load() {
  cv::Mat tmp = cv::imread(path_);
  if (tmp.empty()) {
    spdlog::error("Failed to load image {}", path_);
    return;
  }
  cv::resize(tmp, preview_, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
  sift->detectAndCompute(preview_, cv::Mat(), keypoints_, descriptors_);
  cv::resize(preview_, thumbnail_, cv::Size(kPreviewSize, kPreviewSize), 0, 0,
             cv::INTER_AREA);

  spdlog::info("Loaded {}", path_);
  spdlog::info("Size: {} x {}, Keypoints: {}", preview_.size[1],
               preview_.size[0], keypoints_.size());
}

bool Image::IsLoaded() const { return !preview_.empty(); }

cv::Mat Image::GetFullRes() const { return cv::imread(path_); }
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

}  // namespace xpano::algorithm
