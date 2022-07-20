#pragma once

#include <optional>
#include <vector>

#include <opencv2/core.hpp>

#include "image.h"

namespace xpano::algorithm {

struct Pano {
  std::vector<int> ids;
};

struct Match {
  int id1;
  int id2;
  std::vector<cv::DMatch> matches;
};

std::vector<cv::DMatch> MatchImages(const Image& img1, const Image& img2);

std::vector<Pano> FindPanos(const std::vector<Match>& matches);

std::optional<cv::Mat> Stitch(const std::vector<cv::Mat>& images);

}  // namespace xpano::algorithm
