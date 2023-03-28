#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/stitching.hpp>

#include "xpano/algorithm/image.h"
#include "xpano/constants.h"
#include "xpano/utils/rect.h"

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

std::vector<cv::DMatch> MatchImages(const Image& img1, const Image& img2,
                                    float match_conf);

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

enum class FeatureType { kSift, kOrb };

const auto kFeatureTypes = std::array{FeatureType::kSift, FeatureType::kOrb};

const char* Label(FeatureType feature_type);

struct ProjectionOptions {
  ProjectionType type = ProjectionType::kSpherical;
  float a_param = kDefaultPaniniA;
  float b_param = kDefaultPaniniB;
};

struct StitchOptions {
  ProjectionOptions projection;
  FeatureType feature = FeatureType::kSift;
  float match_conf = kDefaultMatchConf;
};

struct StitchResult {
  cv::Stitcher::Status status;
  cv::Mat pano;
  cv::Mat mask;
};

StitchResult Stitch(const std::vector<cv::Mat>& images, StitchOptions options,
                    bool return_pano_mask);

std::string ToString(cv::Stitcher::Status& status);

std::optional<utils::RectRRf> FindLargestCrop(const cv::Mat& mask);

enum class InpaintingMethod {
  kNavierStokes,
  kTelea,
};

const auto kInpaintingMethods =
    std::array{InpaintingMethod::kNavierStokes, InpaintingMethod::kTelea};

const char* Label(InpaintingMethod inpaint_method);

struct InpaintingOptions {
  double radius = kDefaultInpaintingRadius;
  InpaintingMethod method = InpaintingMethod::kTelea;
};

cv::Mat Inpaint(const cv::Mat& pano, const cv::Mat& mask,
                InpaintingOptions options);

}  // namespace xpano::algorithm
