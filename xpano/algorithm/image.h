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

  [[nodiscard]] cv::Mat GetFullRes() const;
  [[nodiscard]] cv::Mat GetThumbnail() const;
  [[nodiscard]] cv::Mat GetPreview() const;
  [[nodiscard]] float GetAspect() const;
  [[nodiscard]] cv::Mat Draw(bool show_debug) const;
  [[nodiscard]] const std::vector<cv::KeyPoint> &GetKeypoints() const;
  [[nodiscard]] cv::Mat GetDescriptors() const;
  [[nodiscard]] bool IsLoaded() const;

 private:
  std::string path_;
  cv::Mat preview_;
  cv::Mat thumbnail_;

  std::vector<cv::KeyPoint> keypoints_;
  cv::Mat descriptors_;
};

}  // namespace xpano::algorithm
