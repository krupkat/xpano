#pragma once

#include <string>
#include <vector>

#include <imgui.h>
#include <opencv2/core.hpp>

namespace xpano {

struct Coord {
  ImVec2 uv0;
  ImVec2 uv1;
  float aspect;
  int id;
};

class Image {
 public:
  Image() = default;
  explicit Image(std::string path);

  void Load();

  [[nodiscard]] cv::Mat GetPreview() const;
  [[nodiscard]] cv::Mat GetImageData() const;
  [[nodiscard]] float GetAspect() const;
  [[nodiscard]] cv::Mat Draw() const;
  [[nodiscard]] const std::vector<cv::KeyPoint> &GetKeypoints() const;
  [[nodiscard]] cv::Mat GetDescriptors() const;

 private:
  std::string path_;
  cv::Mat image_data_;
  cv::Mat preview_;

  std::vector<cv::KeyPoint> keypoints_;
  cv::Mat descriptors_;
};

}  // namespace xpano
