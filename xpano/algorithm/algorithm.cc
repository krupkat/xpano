// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Vaibhav Sharma
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/algorithm/algorithm.h"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/photo.hpp>
#include <opencv2/stitching.hpp>
#include <opencv2/stitching/detail/matchers.hpp>

#include "xpano/algorithm/auto_crop.h"
#include "xpano/algorithm/blenders.h"
#include "xpano/algorithm/image.h"
#include "xpano/algorithm/stitcher.h"
#include "xpano/utils/disjoint_set.h"
#include "xpano/utils/rect.h"
#include "xpano/utils/threadpool.h"
#include "xpano/utils/vec.h"

namespace xpano::algorithm {

namespace {
void InsertInOrder(int value, std::vector<int>* vec) {
  auto iter = std::lower_bound(vec->begin(), vec->end(), value);
  vec->insert(iter, value);
}
cv::Ptr<cv::WarperCreator> PickWarper(ProjectionOptions options) {
  cv::Ptr<cv::WarperCreator> warper_creator;
  switch (options.type) {
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
    case ProjectionType::kPanini:
      warper_creator =
          cv::makePtr<cv::PaniniWarper>(options.a_param, options.b_param);
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

std::optional<cv::RotateFlags> GetRotationFlags(
    WaveCorrectionType wave_correction,
    cv::detail::WaveCorrectKind wave_correction_auto) {
  // have to rotate clockwise in case vertical wave correction was either
  // selected by user or autodetected
  switch (wave_correction) {
    case WaveCorrectionType::kOff:
      [[fallthrough]];
    case WaveCorrectionType::kHorizontal:
      return {};
    case WaveCorrectionType::kVertical:
      return cv::ROTATE_90_CLOCKWISE;
    case WaveCorrectionType::kAuto:
      break;
  }
  if (wave_correction_auto == cv::detail::WAVE_CORRECT_VERT) {
    return cv::ROTATE_90_CLOCKWISE;
  }
  return {};
}

cv::Ptr<cv::FeatureDetector> PickFeaturesFinder(FeatureType feature) {
  switch (feature) {
    case FeatureType::kSift:
      return cv::SIFT::create();
    case FeatureType::kOrb:
      return cv::ORB::create();
    default:
      return nullptr;
  }
}

cv::detail::WaveCorrectKind PickWaveCorrectKind(
    WaveCorrectionType wave_correction) {
  switch (wave_correction) {
    case WaveCorrectionType::kAuto:
      return cv::detail::WAVE_CORRECT_AUTO;
    case WaveCorrectionType::kHorizontal:
      return cv::detail::WAVE_CORRECT_HORIZ;
    case WaveCorrectionType::kVertical:
      return cv::detail::WAVE_CORRECT_VERT;
    default:
      return cv::detail::WAVE_CORRECT_AUTO;
  }
}

cv::Ptr<cv::detail::Blender> PickBlender(BlendingMethod blending_method,
                                         utils::mt::Threadpool* threadpool) {
  switch (blending_method) {
    case BlendingMethod::kOpenCV: {
      return cv::makePtr<blenders::MultiBandOpenCV>();
    }
    case BlendingMethod::kMultiblend: {
      if constexpr (blenders::MultiblendEnabled()) {
        return cv::makePtr<blenders::Multiblend>(threadpool);
      }
      throw std::runtime_error(
          "Multiblend is not supported in this build of xpano");
    }
    default:
      return nullptr;
  }
}

}  // namespace

Match MatchImages(int img1_id, int img2_id, const Image& img1,
                  const Image& img2, float match_conf) {
  if (img1.GetKeypoints().empty() || img2.GetKeypoints().empty()) {
    return {};
  }

  // KNN MATCH, K = 2
  const cv::FlannBasedMatcher matcher;
  std::vector<std::vector<cv::DMatch>> matches;
  matcher.knnMatch(img1.GetDescriptors(), img2.GetDescriptors(), matches, 2);

  // FILTER BY FIRST/SECOND RATIO
  std::vector<cv::DMatch> good_matches;
  for (const auto& match : matches) {
    if (match[0].distance < (1.0f - match_conf) * match[1].distance) {
      good_matches.push_back(match[0]);
    }
  }

  if (good_matches.size() < 4) {
    return {};
  }

  // ESTIMATE HOMOGRAPHY
  const int num_good_matches = static_cast<int>(good_matches.size());
  cv::Mat src_points(1, num_good_matches, CV_32FC2);
  cv::Mat dst_points(1, num_good_matches, CV_32FC2);
  cv::Mat dst_points_proj;
  int idx = 0;
  for (const cv::DMatch& match : good_matches) {
    src_points.at<cv::Vec2f>(0, idx) = img1.GetKeypoints()[match.queryIdx].pt;
    dst_points.at<cv::Vec2f>(0, idx) = img2.GetKeypoints()[match.trainIdx].pt;
    idx++;
  }
  const cv::Mat h_mat =
      cv::findHomography(src_points, dst_points, cv::RANSAC, 3);
  if (h_mat.empty()) {
    return {};
  }
  perspectiveTransform(src_points, dst_points_proj, h_mat);

  // FILTER OUTLIERS
  std::vector<cv::DMatch> inliers;
  double total_shift = 0.0f;

  for (int i = 0; i < good_matches.size(); i++) {
    const cv::Vec2f proj_diff =
        dst_points.at<cv::Vec2f>(0, i) - dst_points_proj.at<cv::Vec2f>(0, i);
    if (norm(proj_diff) < 3) {
      inliers.push_back(good_matches[i]);

      const cv::Vec2f diff =
          dst_points.at<cv::Vec2f>(0, i) - src_points.at<cv::Vec2f>(0, i);
      total_shift += norm(diff);
    }
  }

  const int max_size =
      std::max(img1.GetPreviewLongerSide(), img2.GetPreviewLongerSide());
  const auto avg_shift = static_cast<float>(
      total_shift / static_cast<double>(inliers.size()) / max_size);
  return {img1_id, img2_id, inliers, avg_shift};
}

std::vector<Pano> FindPanos(const std::vector<Match>& matches,
                            int match_threshold, float min_shift) {
  auto pano_ds = utils::DisjointSet();

  std::unordered_set<int> images_in_panos;
  for (const auto& match : matches) {
    if (match.matches.size() >= match_threshold &&
        match.avg_shift >= min_shift) {
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
    const int root = pano_ds.Find(image_id);
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

StitchResult Stitch(const std::vector<cv::Mat>& images,
                    const std::optional<Cameras>& cameras,
                    StitchUserOptions user_options, StitchOptions options) {
  auto stitcher = stitcher::Stitcher::Create(cv::Stitcher::PANORAMA);
  stitcher->SetWarper(PickWarper(user_options.projection));
  stitcher->SetFeaturesFinder(PickFeaturesFinder(user_options.feature));
  stitcher->SetFeaturesMatcher(cv::makePtr<cv::detail::BestOf2NearestMatcher>(
      false, user_options.match_conf));
  stitcher->SetWaveCorrection(user_options.wave_correction !=
                              WaveCorrectionType::kOff);
  if (stitcher->WaveCorrection()) {
    stitcher->SetWaveCorrectKind(
        PickWaveCorrectKind(user_options.wave_correction));
  }
  stitcher->SetBlender(PickBlender(user_options.blending_method,
                                   options.threads_for_multiblend));
  stitcher->SetProgressMonitor(options.progress_monitor);

  cv::Mat pano;
  stitcher::Status status;

  if (cameras &&
      cameras->wave_correction_user == user_options.wave_correction) {
    stitcher->SetWaveCorrectKind(cameras->wave_correction_auto);
    stitcher->SetTransform(images, cameras->cameras);
    status = stitcher->ComposePanorama(pano);
  } else {
    status = stitcher->Stitch(images, pano);
  }

  if (status != stitcher::Status::kSuccess) {
    return {status, {}, {}};
  }

  cv::Mat mask;
  if (options.return_pano_mask) {
    stitcher->ResultMask().copyTo(mask);
  }

  if (auto rotate = GetRotationFlags(user_options.wave_correction,
                                     stitcher->WaveCorrectKind());
      rotate) {
    cv::rotate(pano, pano, *rotate);
    if (options.return_pano_mask) {
      cv::rotate(mask, mask, *rotate);
    }
  }
  auto result_cameras =
      Cameras{stitcher->Cameras(), user_options.wave_correction,
              stitcher->WaveCorrectKind()};
  return {status, pano, mask, std::move(result_cameras)};
}

int StitchTasksCount(int num_images) {
  return 1 +           // find features
         1 +           // match features
         1 +           // estimate homography
         1 +           // bundle adjustment
         1 +           // prepare seams
         1 +           // find seams
         num_images +  // compose
         1 +           // blend
         1;            // end
}

std::string ToString(stitcher::Status& status) {
  switch (status) {
    case stitcher::Status::kSuccess:
      return "OK";
    case stitcher::Status::kCancelled:
      return "Cancelled";
    case stitcher::Status::kErrNeedMoreImgs:
      return "ERR_NEED_MORE_IMGS";
    case stitcher::Status::kErrHomographyEstFail:
      return "ERR_HOMOGRAPHY_EST_FAIL";
    case stitcher::Status::kErrCameraParamsAdjustFail:
      return "ERR_CAMERA_PARAMS_ADJUST_FAIL";
    default:
      return "ERR_UNKNOWN";
  }
}

std::optional<utils::RectRRf> FindLargestCrop(const cv::Mat& mask) {
  std::optional<utils::RectPPi> largest_rect = crop::FindLargestCrop(mask);
  if (!largest_rect) {
    return {};
  }
  auto image_end = utils::Point2i{mask.cols, mask.rows};
  return Rect(largest_rect->start / image_end, largest_rect->end / image_end);
}

cv::Mat Inpaint(const cv::Mat& pano, const cv::Mat& mask,
                InpaintingOptions options) {
  cv::Mat result;
  int method = cv::INPAINT_TELEA;
  if (options.method == InpaintingMethod::kNavierStokes) {
    method = cv::INPAINT_NS;
  }
  cv::inpaint(pano, mask, result, options.radius, method);
  return result;
}

Pano SinglePano(int size) {
  Pano pano;
  pano.ids.resize(size);
  std::iota(pano.ids.begin(), pano.ids.end(), 0);
  return pano;
}

Cameras Rotate(const Cameras& cameras, float angle) {
  cv::Mat rot_vec = angle * cv::Mat {0.0f, 0.0f, 1.0f};
  cv::Mat rot_mat;
  cv::Rodrigues(rot_vec, rot_mat);

  Cameras rotated = cameras;
  for (auto& camera : rotated.cameras) {
    camera.R = rot_mat * camera.R;
  }

  return rotated;
}

}  // namespace xpano::algorithm
