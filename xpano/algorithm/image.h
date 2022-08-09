#pragma once

#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace xpano::algorithm {

class Image {
 public:
  Image() = default;
  explicit Image(std::string path);

  void Load();

  [[nodiscard]] cv::Mat GetPreview() const;
  [[nodiscard]] cv::Mat GetImageData() const;
  [[nodiscard]] float GetAspect() const;
  [[nodiscard]] cv::Mat Draw(bool show_debug) const;
  [[nodiscard]] const std::vector<cv::KeyPoint> &GetKeypoints() const;
  [[nodiscard]] cv::Mat GetDescriptors() const;

 private:
  std::string path_;
  cv::Mat image_data_;
  cv::Mat preview_;

  std::vector<cv::KeyPoint> keypoints_;
  cv::Mat descriptors_;
};

}  // namespace xpano::algorithm
