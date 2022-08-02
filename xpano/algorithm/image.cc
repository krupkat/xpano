#include "algorithm/image.h"

#include <string>
#include <utility>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <SDL.h>

#include "constants.h"

namespace xpano::algorithm {
namespace {
thread_local cv::Ptr<cv::Feature2D> sift = cv::SIFT::create(kNumFeatures);
}  // namespace

Image::Image(std::string path) : path_(std::move(path)) {}

void Image::Load() {
  cv::Mat tmp = cv::imread(path_);
  cv::resize(tmp, image_data_, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
  sift->detectAndCompute(image_data_, cv::Mat(), keypoints_, descriptors_);
  cv::resize(image_data_, preview_, cv::Size(kPreviewSize, kPreviewSize), 0, 0,
             cv::INTER_AREA);

  SDL_Log("Loaded %s\nSize: %d x %d\nKeypoints: %d", path_.c_str(),
          image_data_.size[1], image_data_.size[0], keypoints_.size());
}

cv::Mat Image::GetPreview() const { return preview_; }
cv::Mat Image::GetImageData() const { return image_data_; }

float Image::GetAspect() const {
  auto width = static_cast<float>(image_data_.size[1]);
  auto height = static_cast<float>(image_data_.size[0]);
  return width / height;
}

cv::Mat Image::Draw() const {
  cv::Mat tmp;
  cv::drawKeypoints(image_data_, keypoints_, tmp, cv::Scalar::all(-1),
                    cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
  return tmp;
}

const std::vector<cv::KeyPoint>& Image::GetKeypoints() const {
  return keypoints_;
}

cv::Mat Image::GetDescriptors() const { return descriptors_; }

}  // namespace xpano::algorithm
