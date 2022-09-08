#pragma once

#include <string>
#include <utility>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/stitching.hpp>

#include "xpano/algorithm/image.h"

namespace xpano::algorithm {

struct Pano {
  std::vector<int> ids;
  bool exported = false;
};

struct Match {
  int id1;
  int id2;
  std::vector<cv::DMatch> matches;
};

std::vector<cv::DMatch> MatchImages(const Image& img1, const Image& img2);

std::vector<Pano> FindPanos(const std::vector<Match>& matches,
                            int match_threshold);

enum class ProjectionType {
  kPerspective,
  kCylindrical,
  kSpherical,
  kFisheye,
  kStereographic,
  kCompressedRectilinear,
  kCompressedRectilinearPortrait,
  kPanini,
  kPaniniPortrait,
  kMercator,
  kTransverseMercator
};

const auto kProjectionTypes =
    std::array{ProjectionType::kPerspective,
               ProjectionType::kCylindrical,
               ProjectionType::kSpherical,
               ProjectionType::kFisheye,
               ProjectionType::kStereographic,
               ProjectionType::kCompressedRectilinear,
               ProjectionType::kCompressedRectilinearPortrait,
               ProjectionType::kPanini,
               ProjectionType::kPaniniPortrait,
               ProjectionType::kMercator,
               ProjectionType::kTransverseMercator};

const char* Label(ProjectionType projection_type);

bool HasAdvancedParameters(ProjectionType projection_type);

struct ProjectionOptions {
  algorithm::ProjectionType projection_type =
      algorithm::ProjectionType::kSpherical;
  float a_param = 2.0f;
  float b_param = 1.0f;
};

std::pair<cv::Stitcher::Status, cv::Mat> Stitch(
    const std::vector<cv::Mat>& images, ProjectionOptions options);

std::string ToString(cv::Stitcher::Status& status);

}  // namespace xpano::algorithm
