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

struct ProjectionOptions {
  ProjectionType type = ProjectionType::kSpherical;
  float a_param = kDefaultPaniniA;
  float b_param = kDefaultPaniniB;
};

enum class InpaintingMethod {
  kNavierStokes,
  kTelea,
};

const auto kInpaintingMethods =
    std::array{InpaintingMethod::kNavierStokes, InpaintingMethod::kTelea};

struct InpaintingOptions {
  double radius = kDefaultInpaintingRadius;
  InpaintingMethod method = InpaintingMethod::kTelea;
};

using CameraParams = std::vector<cv::detail::CameraParams>;

struct PanoModifications {
  ProjectionOptions projection;

  std::optional<utils::RectRRf> crop;
  std::optional<CameraParams> cameras;
  std::optional<InpaintingOptions> inpainting;
};

struct Pano {
  std::vector<int> ids;
  bool exported = false;
  PanoModifications modifications;
};

struct Match {
  int id1;
  int id2;
  std::vector<cv::DMatch> matches;
};

std::vector<cv::DMatch> MatchImages(const Image& img1, const Image& img2);

std::vector<Pano> FindPanos(const std::vector<Match>& matches,
                            int match_threshold);

const char* Label(ProjectionType projection_type);

bool HasAdvancedParameters(ProjectionType projection_type);

struct StitchOptions {
  ProjectionOptions projection;
  bool return_pano_mask = false;
  std::optional<std::vector<cv::detail::CameraParams>> cameras;
};

struct StitchResult {
  cv::Stitcher::Status status;
  cv::Mat pano;
  cv::Mat mask;
  std::vector<cv::detail::CameraParams> cameras;
};

StitchResult Stitch(const std::vector<cv::Mat>& images, StitchOptions options);

std::string ToString(cv::Stitcher::Status& status);

std::optional<utils::RectRRf> FindLargestCrop(const cv::Mat& mask);

const char* Label(InpaintingMethod inpaint_method);

cv::Mat Inpaint(const cv::Mat& pano, const cv::Mat& mask,
                InpaintingOptions options);

}  // namespace xpano::algorithm
