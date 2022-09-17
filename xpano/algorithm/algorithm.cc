#include "xpano/algorithm/algorithm.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/stitching.hpp>

#include "xpano/algorithm/image.h"
#include "xpano/utils/disjoint_set.h"
#include "xpano/utils/rect.h"
#include "xpano/utils/vec.h"

namespace xpano::algorithm {

namespace {
void InsertInOrder(int value, std::vector<int>* vec) {
  auto iter = std::lower_bound(vec->begin(), vec->end(), value);
  vec->insert(iter, value);
}
cv::Ptr<cv::WarperCreator> PickWarper(ProjectionOptions options) {
  cv::Ptr<cv::WarperCreator> warper_creator;
  switch (options.projection_type) {
    case ProjectionType::kPerspective:
      warper_creator = cv::makePtr<cv::PlaneWarper>();
      break;
    case ProjectionType::kCylindrical:
      warper_creator = cv::makePtr<cv::CylindricalWarper>();
      break;
    case ProjectionType::kSpherical:
      warper_creator = cv::makePtr<cv::SphericalWarper>();
      break;
    case ProjectionType::kFisheye:
      warper_creator = cv::makePtr<cv::FisheyeWarper>();
      break;
    case ProjectionType::kStereographic:
      warper_creator = cv::makePtr<cv::StereographicWarper>();
      break;
    case ProjectionType::kCompressedRectilinear:
      warper_creator = cv::makePtr<cv::CompressedRectilinearWarper>(
          options.a_param, options.b_param);
      break;
    case ProjectionType::kCompressedRectilinearPortrait:
      warper_creator = cv::makePtr<cv::CompressedRectilinearPortraitWarper>(
          options.a_param, options.b_param);
      break;
    case ProjectionType::kPanini:
      warper_creator =
          cv::makePtr<cv::PaniniWarper>(options.a_param, options.b_param);
      break;
    case ProjectionType::kPaniniPortrait:
      warper_creator = cv::makePtr<cv::PaniniPortraitWarper>(options.a_param,
                                                             options.b_param);
      break;
    case ProjectionType::kMercator:
      warper_creator = cv::makePtr<cv::MercatorWarper>();
      break;
    case ProjectionType::kTransverseMercator:
      warper_creator = cv::makePtr<cv::TransverseMercatorWarper>();
      break;
  }
  return warper_creator;
}

}  // namespace

std::vector<cv::DMatch> MatchImages(const Image& img1, const Image& img2) {
  if (img1.GetKeypoints().empty() || img2.GetKeypoints().empty()) {
    return {};
  }

  // KNN MATCH, K = 2
  cv::FlannBasedMatcher matcher;
  std::vector<std::vector<cv::DMatch>> matches;
  matcher.knnMatch(img1.GetDescriptors(), img2.GetDescriptors(), matches, 2);

  // FILTER BY FIRST/SECOND RATIO
  std::vector<cv::DMatch> good_matches;
  for (const auto& match : matches) {
    double ratio = match[0].distance / match[1].distance;
    if (ratio < 0.8) {
      good_matches.push_back(match[0]);
    }
  }

  if (good_matches.size() < 4) {
    return {};
  }

  // ESTIMATE HOMOGRAPHY
  int num_good_matches = static_cast<int>(good_matches.size());
  cv::Mat src_points(1, num_good_matches, CV_32FC2);
  cv::Mat dst_points(1, num_good_matches, CV_32FC2);
  cv::Mat dst_points_proj;
  int idx = 0;
  for (cv::DMatch& match : good_matches) {
    src_points.at<cv::Vec2f>(0, idx) = img1.GetKeypoints()[match.queryIdx].pt;
    dst_points.at<cv::Vec2f>(0, idx) = img2.GetKeypoints()[match.trainIdx].pt;
    idx++;
  }
  cv::Mat h_mat = cv::findHomography(src_points, dst_points, cv::RANSAC, 3);
  if (h_mat.empty()) {
    return {};
  }
  perspectiveTransform(src_points, dst_points_proj, h_mat);

  // FILTER OUTLIERS
  std::vector<cv::DMatch> inliers;
  for (int i = 0; i < good_matches.size(); i++) {
    cv::Vec2f diff =
        dst_points.at<cv::Vec2f>(0, i) - dst_points_proj.at<cv::Vec2f>(0, i);
    if (norm(diff) < 3) {
      inliers.push_back(good_matches[i]);
    }
  }

  return inliers;
}

std::vector<Pano> FindPanos(const std::vector<Match>& matches,
                            int match_threshold) {
  auto pano_ds = utils::DisjointSet();

  std::unordered_set<int> images_in_panos;
  for (const auto& match : matches) {
    if (match.matches.size() > match_threshold) {
      pano_ds.Union(match.id1, match.id2);
      images_in_panos.insert(match.id1);
      images_in_panos.insert(match.id2);
    }
  }

  if (images_in_panos.empty()) {
    return {};
  }

  std::unordered_map<int, Pano> pano_map;
  for (auto image_id : images_in_panos) {
    int root = pano_ds.Find(image_id);
    if (auto pano = pano_map.find(root); pano != pano_map.end()) {
      InsertInOrder(image_id, &pano->second.ids);
    } else {
      pano_map.emplace(root, Pano{.ids = {image_id}});
    }
  }

  std::vector<Pano> result;
  std::transform(pano_map.begin(), pano_map.end(), std::back_inserter(result),
                 [](const auto& pano) { return pano.second; });
  std::sort(result.begin(), result.end(), [](const Pano& lhs, const Pano& rhs) {
    return lhs.ids[0] < rhs.ids[0];
  });
  return result;
}

std::tuple<cv::Stitcher::Status, cv::Mat, cv::Mat> Stitch(
    const std::vector<cv::Mat>& images, ProjectionOptions options) {
  auto stitcher = cv::Stitcher::create(cv::Stitcher::PANORAMA);
  auto warper_creator = PickWarper(options);
  stitcher->setWarper(warper_creator);

  cv::Mat out;
  auto status = stitcher->stitch(images, out);

  if (status != cv::Stitcher::OK) {
    return {status, {}, {}};
  }

  cv::Mat mask;
  stitcher->resultMask().copyTo(mask);

  if (options.projection_type == ProjectionType::kStereographic) {
    cv::rotate(out, out, cv::ROTATE_90_COUNTERCLOCKWISE);
    cv::rotate(mask, mask, cv::ROTATE_90_COUNTERCLOCKWISE);
  }

  if (options.projection_type == ProjectionType::kFisheye) {
    cv::rotate(out, out, cv::ROTATE_90_CLOCKWISE);
    cv::rotate(mask, mask, cv::ROTATE_90_CLOCKWISE);
  }

  return {status, out, mask};
}

std::string ToString(cv::Stitcher::Status& status) {
  switch (status) {
    case cv::Stitcher::OK:
      return "OK";
    case cv::Stitcher::ERR_NEED_MORE_IMGS:
      return "ERR_NEED_MORE_IMGS";
    case cv::Stitcher::ERR_HOMOGRAPHY_EST_FAIL:
      return "ERR_HOMOGRAPHY_EST_FAIL";
    case cv::Stitcher::ERR_CAMERA_PARAMS_ADJUST_FAIL:
      return "ERR_CAMERA_PARAMS_ADJUST_FAIL";
    default:
      return "ERR_UNKNOWN";
  }
}

// NOLINTBEGIN(bugprone-branch-clone): doesn't work with [[fallthrough]]

bool HasAdvancedParameters(ProjectionType projection_type) {
  switch (projection_type) {
    case ProjectionType::kCompressedRectilinear:
      [[fallthrough]];
    case ProjectionType::kCompressedRectilinearPortrait:
      [[fallthrough]];
    case ProjectionType::kPanini:
      [[fallthrough]];
    case ProjectionType::kPaniniPortrait:
      return true;
    default:
      return false;
  }
}

// NOLINTEND(bugprone-branch-clone)

const char* Label(ProjectionType projection_type) {
  switch (projection_type) {
    case ProjectionType::kPerspective:
      return "Perspective";
    case ProjectionType::kCylindrical:
      return "Cylindrical";
    case ProjectionType::kSpherical:
      return "Spherical";
    case ProjectionType::kFisheye:
      return "Fisheye";
    case ProjectionType::kStereographic:
      return "Stereographic";
    case ProjectionType::kCompressedRectilinear:
      return "CompressedRectilinear";
    case ProjectionType::kCompressedRectilinearPortrait:
      return "CompressedRectilinearPortrait";
    case ProjectionType::kPanini:
      return "Panini";
    case ProjectionType::kPaniniPortrait:
      return "PaniniPortrait";
    case ProjectionType::kMercator:
      return "Mercator";
    case ProjectionType::kTransverseMercator:
      return "TransverseMercator";
    default:
      return "Unknown";
  }
}

utils::RectRRf FindLargestCropRectangle(cv::Mat mask) {
  auto start = utils::Ratio2f{0.0f};
  auto end = utils::Ratio2f{1.0f};

  // find largest area with 0xFF

  return Rect(start, end);
}

}  // namespace xpano::algorithm
