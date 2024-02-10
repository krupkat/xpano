// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-License-Identifier: Apache-2.0
//
///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this
//  license. If you do not agree to this license, do not download, install, copy
//  or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright
//   notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote
//   products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is"
// and any express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular purpose
// are disclaimed. In no event shall the Intel Corporation or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or business
// interruption) however caused and on any theory of liability, whether in
// contract, strict liability, or tort (including negligence or otherwise)
// arising in any way out of the use of this software, even if advised of the
// possibility of such damage.
//

#include "xpano/algorithm/stitcher.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <string_view>
#include <utility>

#include <spdlog/spdlog.h>

#include "xpano/constants.h"
#include "xpano/utils/opencv.h"

namespace xpano::algorithm::stitcher {

namespace {

constexpr unsigned char kMaskValueOn = 0xFF;

using ProgressType = algorithm::ProgressType;

class Timer {
 public:
  Timer() {
    if (Enabled()) {
      start_count_ = cv::getTickCount();
    }
  }

  void Report(std::string_view message) {
    if (Enabled()) {
      const int64 end_count = cv::getTickCount();
      auto elapsed =
          static_cast<double>(end_count - start_count_) / TickFrequency();
      spdlog::trace("{}: {} sec", message, elapsed);
      start_count_ = end_count;
    }
  }

 private:
  static bool Enabled() {
    static const bool kEnabled = spdlog::get_level() <= spdlog::level::trace;
    return kEnabled;
  }

  static double TickFrequency() {
    static const double kTickFrequency = cv::getTickFrequency();
    return kTickFrequency;
  }

  int64 start_count_ = 0;
};

double ComputeWarpScale(const std::vector<cv::detail::CameraParams> &cameras) {
  std::vector<double> focals(cameras.size());
  std::transform(cameras.begin(), cameras.end(), focals.begin(),
                 [](const auto &camera) { return camera.focal; });

  std::sort(focals.begin(), focals.end());
  if (focals.size() % 2 == 1) {
    return focals[focals.size() / 2];
  }
  return (focals[focals.size() / 2 - 1] + focals[focals.size() / 2]) * 0.5;
}

double ComputeWorkScale(const cv::Size &img_size, double registr_resol) {
  if (registr_resol < 0) {
    return 1.0;
  }
  return std::min(1.0, std::sqrt(registr_resol * 1e6 / img_size.area()));
}

double ComputeSeamScale(const cv::Size &img_size, double seam_est_resol) {
  return std::min(1.0, std::sqrt(seam_est_resol * 1e6 / img_size.area()));
}

template <typename TType>
std::vector<TType> Index(const std::vector<TType> &vec,
                         const std::vector<int> &indices) {
  std::vector<TType> subset(indices.size());
  std::transform(indices.begin(), indices.end(), subset.begin(),
                 [&vec](int index) { return vec[index]; });
  return subset;
}

}  // namespace

cv::Ptr<Stitcher> Stitcher::Create(Mode mode) {
  cv::Ptr<Stitcher> stitcher = cv::makePtr<Stitcher>();

  stitcher->SetRegistrationResol(0.6);
  stitcher->SetSeamEstimationResol(0.1);
  stitcher->SetPanoConfidenceThresh(1);
  stitcher->SetSeamFinder(cv::makePtr<cv::detail::GraphCutSeamFinder>(
      cv::detail::GraphCutSeamFinderBase::COST_COLOR));
  stitcher->SetBlender(cv::makePtr<cv::detail::MultiBandBlender>(false));
  stitcher->SetFeaturesFinder(cv::ORB::create());
  stitcher->SetInterpolationFlags(cv::INTER_LINEAR);

  switch (mode) {
    case Mode::PANORAMA:  // PANORAMA is the default
      stitcher->SetEstimator(
          cv::makePtr<cv::detail::HomographyBasedEstimator>());
      stitcher->SetWaveCorrection(true);
      stitcher->SetWaveCorrectKind(cv::detail::WAVE_CORRECT_HORIZ);
      stitcher->SetFeaturesMatcher(
          cv::makePtr<cv::detail::BestOf2NearestMatcher>(false));
      stitcher->SetBundleAdjuster(cv::makePtr<cv::detail::BundleAdjusterRay>());
      stitcher->SetWarper(cv::makePtr<cv::SphericalWarper>());
      stitcher->SetExposureCompensator(
          cv::makePtr<cv::detail::BlocksGainCompensator>());
      break;

    case Mode::SCANS:
      stitcher->SetEstimator(cv::makePtr<cv::detail::AffineBasedEstimator>());
      stitcher->SetWaveCorrection(false);
      stitcher->SetFeaturesMatcher(
          cv::makePtr<cv::detail::AffineBestOf2NearestMatcher>(false, false));
      stitcher->SetBundleAdjuster(
          cv::makePtr<cv::detail::BundleAdjusterAffinePartial>());
      stitcher->SetWarper(cv::makePtr<cv::AffineWarper>());
      stitcher->SetExposureCompensator(
          cv::makePtr<cv::detail::NoExposureCompensator>());
      break;

    default:
      CV_Error(cv::Error::StsBadArg,
               "Invalid stitching mode. Must be one of Stitcher::Mode");
      break;
  }

  return stitcher;
}

Status Stitcher::EstimateTransform(cv::InputArrayOfArrays images,
                                   cv::InputArrayOfArrays masks) {
  images.getUMatVector(imgs_);
  masks.getUMatVector(masks_);

  if (auto status = MatchImages(); status != Status::kSuccess) {
    return status;
  }

  if (auto status = EstimateCameraParams(); status != Status::kSuccess) {
    return status;
  }

  return Status::kSuccess;
}

Status Stitcher::EstimateSeams(std::vector<cv::UMat> *seams) {
  auto seam_timer = Timer();

  std::vector<cv::UMat> masks(imgs_.size());
  std::vector<cv::Point> corners(imgs_.size());
  std::vector<cv::Size> sizes(imgs_.size());

  std::vector<cv::UMat> masks_warped(imgs_.size());
  std::vector<cv::UMat> images_warped(imgs_.size());

  // Prepare image masks
  for (size_t i = 0; i < imgs_.size(); ++i) {
    masks[i].create(seam_est_imgs_[i].size(), CV_8U);
    masks[i].setTo(cv::Scalar::all(kMaskValueOn));
  }

  // Warp images and their masks
  const cv::Ptr<cv::detail::RotationWarper> warper = warper_creater_->create(
      static_cast<float>(warped_image_scale_ * seam_work_aspect_));
  auto seam_cameras = utils::opencv::Scale(cameras_, seam_work_aspect_);
  for (size_t i = 0; i < imgs_.size(); ++i) {
    auto k_float = utils::opencv::ToFloat(seam_cameras[i].K());

    corners[i] =
        warper->warp(seam_est_imgs_[i], k_float, cameras_[i].R, interp_flags_,
                     cv::BORDER_REFLECT, images_warped[i]);
    sizes[i] = images_warped[i].size();

    warper->warp(masks[i], k_float, cameras_[i].R, cv::INTER_NEAREST,
                 cv::BORDER_CONSTANT, masks_warped[i]);
  }

  // Compensate exposure before finding seams
  exposure_comp_->feed(corners, images_warped, masks_warped);
  for (size_t i = 0; i < imgs_.size(); ++i) {
    exposure_comp_->apply(static_cast<int>(i), corners[i], images_warped[i],
                          masks_warped[i]);
  }

  if (Cancelled()) {
    return Status::kCancelled;
  }
  NextTask(ProgressType::kStitchSeamsFind);

  // Find seams
  std::vector<cv::UMat> images_warped_f(imgs_.size());
  for (size_t i = 0; i < imgs_.size(); ++i) {
    images_warped[i].convertTo(images_warped_f[i], CV_32F);
  }
  seam_finder_->find(images_warped_f, corners, masks_warped);

  seam_timer.Report("Finding seams");

  *seams = std::move(masks_warped);
  return Status::kSuccess;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity):
Status Stitcher::ComposePanorama(cv::OutputArray pano) {
  cv::UMat img_warped;
  cv::UMat dilated_mask;
  cv::UMat seam_mask;
  cv::UMat mask;
  cv::UMat mask_warped;

  auto compose_work_aspect = 1.0 / work_scale_;
  auto cameras_scaled = utils::opencv::Scale(cameras_, compose_work_aspect);

  std::vector<cv::Point> corners(imgs_.size());
  std::vector<cv::Size> sizes(imgs_.size());

  cv::Ptr<cv::detail::RotationWarper> warper;
  {
    spdlog::info("Calculating pano size... ");
    NextTask(ProgressType::kStitchComputeRoi);
    auto compute_roi_timer = Timer();

    // Update warped image scale
    auto warp_scale =
        static_cast<float>(warped_image_scale_ * compose_work_aspect);
    warper = warper_creater_->create(warp_scale);

    // Update corners and sizes
    for (size_t i = 0; i < imgs_.size(); ++i) {
      auto k_float = utils::opencv::ToFloat(cameras_scaled[i].K());
      const cv::Rect roi =
          warper->warpRoi(full_img_sizes_[i], k_float, cameras_scaled[i].R);
      corners[i] = roi.tl();
      sizes[i] = roi.size();
    }
    compute_roi_timer.Report(" compute pano size time");
  }
  auto dst_roi = cv::detail::resultRoi(corners, sizes);

  if (dst_roi.width >= kMaxPanoSize || dst_roi.height >= kMaxPanoSize) {
    spdlog::error("Panorama is too large to compute: {}x{}, max size is {}",
                  dst_roi.width, dst_roi.height, kMaxPanoSize);
    return Status::kErrPanoTooLarge;
  }

  std::vector<cv::UMat> masks_warped;
  {
    spdlog::info("Estimating seams... ");
    NextTask(ProgressType::kStitchSeamsPrepare);

    if (auto status = EstimateSeams(&masks_warped);
        status != Status::kSuccess) {
      return status;
    }

    seam_est_imgs_.clear();

    if (Cancelled()) {
      return Status::kCancelled;
    }
  }

  spdlog::info("Compositing...");
  auto compositing_total_timer = Timer();

  blender_->prepare(dst_roi);
  for (size_t img_idx = 0; img_idx < imgs_.size(); ++img_idx) {
    NextTask(ProgressType::kStitchCompose);
    if (auto non_zero = cv::countNonZero(masks_warped[img_idx]);
        non_zero == 0) {
      spdlog::warn("Skipping fully obscured image");
      continue;
    }

    spdlog::trace("Compositing image #{}", indices_[img_idx] + 1);
    auto compositing_timer = Timer();

    cv::UMat img = imgs_[img_idx];
    const cv::Size img_size = img.size();

    const cv::Mat k_float = utils::opencv::ToFloat(cameras_scaled[img_idx].K());

    auto timer = Timer();

    // Warp the current image
    warper->warp(img, k_float, cameras_[img_idx].R, interp_flags_,
                 cv::BORDER_REFLECT, img_warped);
    timer.Report(" warp the current image");

    // Warp the current image mask
    mask.create(img_size, CV_8U);
    mask.setTo(cv::Scalar::all(kMaskValueOn));
    warper->warp(mask, k_float, cameras_[img_idx].R, cv::INTER_NEAREST,
                 cv::BORDER_CONSTANT, mask_warped);
    timer.Report(" warp the current image mask");

    // Compensate exposure
    exposure_comp_->apply(static_cast<int>(img_idx), corners[img_idx],
                          img_warped, mask_warped);
    timer.Report(" compensate exposure");

    img.release();
    mask.release();

    // Make sure seam mask has proper size
    dilate(masks_warped[img_idx], dilated_mask, cv::Mat());
    resize(dilated_mask, seam_mask, mask_warped.size(), 0, 0,
           cv::INTER_LINEAR_EXACT);

    bitwise_and(seam_mask, mask_warped, mask_warped);
    timer.Report(" other");

    // Blend the current image
    blender_->feed(img_warped, mask_warped, corners[img_idx]);
    timer.Report(" feed time");

    compositing_timer.Report("Compositing ## time");

    if (Cancelled()) {
      return Status::kCancelled;
    }
  }

  NextTask(ProgressType::kStitchBlend);
  auto blend_timer = Timer();

  cv::UMat result;
  blender_->blend(result, result_mask_);
  blend_timer.Report(" blend time");

  compositing_total_timer.Report("Compositing");

  pano.assign(result);

  warp_helper_ = {work_scale_, corners, sizes, full_img_sizes_,
                  std::move(warper)};

  EndMonitoring();
  return Status::kSuccess;
}

Status Stitcher::Stitch(cv::InputArrayOfArrays images, cv::OutputArray pano) {
  return Stitch(images, cv::noArray(), pano);
}

Status Stitcher::Stitch(cv::InputArrayOfArrays images,
                        cv::InputArrayOfArrays masks, cv::OutputArray pano) {
  const Status status = EstimateTransform(images, masks);
  if (status != Status::kSuccess) {
    return status;
  }
  return ComposePanorama(pano);
}

Status Stitcher::MatchImages() {
  if (static_cast<int>(imgs_.size()) < 2) {
    spdlog::error("Need more images");
    return Status::kErrNeedMoreImgs;
  }

  work_scale_ = ComputeWorkScale(imgs_[0].size(), registr_resol_);
  seam_scale_ = ComputeSeamScale(imgs_[0].size(), seam_est_resol_);
  seam_work_aspect_ = seam_scale_ / work_scale_;

  features_.resize(imgs_.size());
  seam_est_imgs_.resize(imgs_.size());
  full_img_sizes_.resize(imgs_.size());

  spdlog::info("Finding features...");
  NextTask(ProgressType::kStitchFindFeatures);
  auto timer = Timer();

  std::vector<cv::UMat> feature_find_imgs(imgs_.size());
  std::vector<cv::UMat> feature_find_masks(masks_.size());

  for (size_t i = 0; i < imgs_.size(); ++i) {
    full_img_sizes_[i] = imgs_[i].size();
    if (registr_resol_ < 0) {
      feature_find_imgs[i] = imgs_[i];
    } else {
      resize(imgs_[i], feature_find_imgs[i], cv::Size(), work_scale_,
             work_scale_, cv::INTER_LINEAR_EXACT);
    }

    if (!masks_.empty()) {
      resize(masks_[i], feature_find_masks[i], cv::Size(), work_scale_,
             work_scale_, cv::INTER_NEAREST);
    }
    features_[i].img_idx = static_cast<int>(i);

    cv::resize(imgs_[i], seam_est_imgs_[i], cv::Size(), seam_scale_,
               seam_scale_, cv::INTER_LINEAR_EXACT);
  }

  // find features possibly in parallel
  cv::detail::computeImageFeatures(features_finder_, feature_find_imgs,
                                   features_, feature_find_masks);

  // Do it to save memory
  feature_find_imgs.clear();
  feature_find_masks.clear();

  timer.Report("Finding features");
  if (Cancelled()) {
    return Status::kCancelled;
  }

  spdlog::info("Pairwise matching");
  NextTask(ProgressType::kStitchMatchFeatures);

  (*features_matcher_)(features_, pairwise_matches_, matching_mask_);
  features_matcher_->collectGarbage();

  timer.Report("Pairwise matching");
  if (Cancelled()) {
    return Status::kCancelled;
  }

  // Leave only images we are sure are from the same panorama
  indices_ = cv::detail::leaveBiggestComponent(
      features_, pairwise_matches_, static_cast<float>(conf_thresh_));

  if (indices_.size() < 2) {
    spdlog::error("Need more images");
    return Status::kErrNeedMoreImgs;
  }

  seam_est_imgs_ = Index(seam_est_imgs_, indices_);
  imgs_ = Index(imgs_, indices_);
  full_img_sizes_ = Index(full_img_sizes_, indices_);

  return Status::kSuccess;
}

Status Stitcher::EstimateCameraParams() {
  NextTask(ProgressType::kStitchEstimateHomography);
  // estimate homography in global frame
  if (!(*estimator_)(features_, pairwise_matches_, cameras_)) {
    return Status::kErrHomographyEstFail;
  }

  if (Cancelled()) {
    return Status::kCancelled;
  }
  NextTask(ProgressType::kStitchBundleAdjustment);

  for (auto &camera : cameras_) {
    camera.R = utils::opencv::ToFloat(camera.R);
  }

  bundle_adjuster_->setConfThresh(conf_thresh_);
  if (!(*bundle_adjuster_)(features_, pairwise_matches_, cameras_)) {
    return Status::kErrCameraParamsAdjustFail;
  }

  if (Cancelled()) {
    return Status::kCancelled;
  }

  warped_image_scale_ = ComputeWarpScale(cameras_);

  if (do_wave_correct_) {
    std::vector<cv::Mat> rmats;
    rmats.reserve(cameras_.size());

    for (auto &camera : cameras_) {
      rmats.push_back(camera.R.clone());
    }
    if (wave_correct_kind_ == cv::detail::WAVE_CORRECT_AUTO) {
      wave_correct_kind_ = cv::detail::autoDetectWaveCorrectKind(rmats);
    }
    cv::detail::waveCorrect(rmats, wave_correct_kind_);
    for (size_t i = 0; i < cameras_.size(); ++i) {
      cameras_[i].R = rmats[i];
    }

    if (wave_correct_kind_ == cv::detail::WAVE_CORRECT_VERT &&
        warper_creater_portrait_) {
      warper_creater_ = warper_creater_portrait_;
    }
  }

  return Status::kSuccess;
}

Status Stitcher::SetTransform(
    cv::InputArrayOfArrays images,
    const std::vector<cv::detail::CameraParams> &cameras) {
  std::vector<int> component(images.total());
  std::iota(component.begin(), component.end(), 0);

  return SetTransform(images, cameras, component);
}

Status Stitcher::SetTransform(
    cv::InputArrayOfArrays images,
    const std::vector<cv::detail::CameraParams> &cameras,
    const std::vector<int> &component) {
  images.getUMatVector(imgs_);
  masks_.clear();

  if (imgs_.size() < 2 || component.size() < 2) {
    spdlog::error("Need more images");
    return Status::kErrNeedMoreImgs;
  }

  work_scale_ = ComputeWorkScale(imgs_[0].size(), registr_resol_);
  seam_scale_ = ComputeSeamScale(imgs_[0].size(), seam_est_resol_);
  seam_work_aspect_ = seam_scale_ / work_scale_;

  seam_est_imgs_.resize(imgs_.size());
  full_img_sizes_.resize(imgs_.size());

  for (size_t i = 0; i < imgs_.size(); ++i) {
    full_img_sizes_[i] = imgs_[i].size();

    resize(imgs_[i], seam_est_imgs_[i], cv::Size(), seam_scale_, seam_scale_,
           cv::INTER_LINEAR_EXACT);
  }

  features_.clear();
  pairwise_matches_.clear();

  indices_ = component;
  seam_est_imgs_ = Index(seam_est_imgs_, indices_);
  imgs_ = Index(imgs_, indices_);
  full_img_sizes_ = Index(full_img_sizes_, indices_);

  cameras_ = cameras;
  warped_image_scale_ = ComputeWarpScale(cameras_);

  return Status::kSuccess;
}

bool Stitcher::Cancelled() const {
  return (monitor_ != nullptr) ? monitor_->IsCancelled() : false;
}

void Stitcher::NextTask(algorithm::ProgressType task) {
  if (monitor_ != nullptr) {
    monitor_->NotifyTaskDone();
    monitor_->SetTaskType(task);
  }
}

void Stitcher::EndMonitoring() {
  if (monitor_ != nullptr) {
    monitor_->NotifyTaskDone();
  }
}

}  // namespace xpano::algorithm::stitcher
