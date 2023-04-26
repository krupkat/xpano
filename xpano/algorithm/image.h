#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace xpano::algorithm {

struct ImageLoadOptions {
  int preview_longer_side = 0;
  bool compute_keypoints = true;
};

class Image {
 public:
  Image() = default;
  explicit Image(std::filesystem::path path);

  void Load(ImageLoadOptions options);

  [[nodiscard]] cv::Mat GetFullRes() const;
  [[nodiscard]] cv::Mat GetThumbnail() const;
  [[nodiscard]] cv::Mat GetPreview() const;
  [[nodiscard]] float GetAspect() const;
  [[nodiscard]] cv::Mat Draw(bool show_debug) const;
  [[nodiscard]] const std::vector<cv::KeyPoint> &GetKeypoints() const;
  [[nodiscard]] cv::Mat GetDescriptors() const;
  [[nodiscard]] bool IsLoaded() const;
  [[nodiscard]] bool IsRaw() const;
  [[nodiscard]] std::string PanoName() const;

 private:
  std::filesystem::path path_;
  cv::Mat preview_;
  cv::Mat thumbnail_;

  std::vector<cv::KeyPoint> keypoints_;
  cv::Mat descriptors_;
  bool is_raw_ = false;
};

}  // namespace xpano::algorithm
