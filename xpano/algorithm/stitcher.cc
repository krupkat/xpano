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

#include <string_view>

#include <spdlog/spdlog.h>

namespace xpano::algorithm::stitcher {

namespace {

constexpr unsigned char kMaskValueOn = 0xFF;

class Timer {
 public:
  Timer() {
    if (Enabled()) {
      start_count_ = cv::getTickCount();
    }
  }

  void Report(std::string_view message) {
    if (Enabled()) {
      int64 end_count = cv::getTickCount();
      auto elapsed =
          static_cast<double>(end_count - start_count_) / TickFrequency();
      spdlog::trace("{}: {} sec", message, elapsed);
      start_count_ = end_count;
    }
  }

 private:
  static bool Enabled() {
    static bool enabled = spdlog::get_level() <= spdlog::level::trace;
    return enabled;
  }

  static double TickFrequency() {
    static double tick_frequency = cv::getTickFrequency();
    return tick_frequency;
  }

  int64 start_count_;
};

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

  stitcher->work_scale_ = 1;
  stitcher->seam_scale_ = 1;
  stitcher->seam_work_aspect_ = 1;
  stitcher->warped_image_scale_ = 1;

  switch (mode) {
    case Mode::PANORAMA:  // PANORAMA is the default
      // mostly already setup
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

Stitcher::Status Stitcher::EstimateTransform(cv::InputArrayOfArrays images,
                                             cv::InputArrayOfArrays masks) {
  images.getUMatVector(imgs_);
  masks.getUMatVector(masks_);

  Status status;

  if ((status = MatchImages()) != Status::OK) {
    return status;
  }

  if ((status = EstimateCameraParams()) != Status::OK) {
    return status;
  }

  return Status::OK;
}

Stitcher::Status Stitcher::ComposePanorama(cv::OutputArray pano) {
  return ComposePanorama(std::vector<cv::UMat>(), pano);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity):
Stitcher::Status Stitcher::ComposePanorama(cv::InputArrayOfArrays images,
                                           cv::OutputArray pano) {
  spdlog::info("Warping images (auxiliary)... ");

  std::vector<cv::UMat> imgs;
  images.getUMatVector(imgs);
  if (!imgs.empty()) {
    CV_Assert(imgs.size() == imgs_.size());

    cv::UMat img;
    seam_est_imgs_.resize(imgs.size());

    for (size_t i = 0; i < imgs.size(); ++i) {
      imgs_[i] = imgs[i];
      resize(imgs[i], img, cv::Size(), seam_scale_, seam_scale_,
             cv::INTER_LINEAR_EXACT);
      seam_est_imgs_[i] = img.clone();
    }

    std::vector<cv::UMat> seam_est_imgs_subset;
    std::vector<cv::UMat> imgs_subset;

    for (int indice : indices_) {
      imgs_subset.push_back(imgs_[indice]);
      seam_est_imgs_subset.push_back(seam_est_imgs_[indice]);
    }

    seam_est_imgs_ = seam_est_imgs_subset;
    imgs_ = imgs_subset;
  }

  auto warp_timer = Timer();

  std::vector<cv::Point> corners(imgs_.size());
  std::vector<cv::UMat> masks_warped(imgs_.size());
  std::vector<cv::UMat> images_warped(imgs_.size());
  std::vector<cv::Size> sizes(imgs_.size());
  std::vector<cv::UMat> masks(imgs_.size());

  // Prepare image masks
  for (size_t i = 0; i < imgs_.size(); ++i) {
    masks[i].create(seam_est_imgs_[i].size(), CV_8U);
    masks[i].setTo(cv::Scalar::all(kMaskValueOn));
  }

  // Warp images and their masks
  cv::Ptr<cv::detail::RotationWarper> warper = warper_creater_->create(
      static_cast<float>(warped_image_scale_ * seam_work_aspect_));
  for (size_t i = 0; i < imgs_.size(); ++i) {
    cv::Mat_<float> k_float;
    cameras_[i].K().convertTo(k_float, CV_32F);
    k_float(0, 0) *= static_cast<float>(seam_work_aspect_);
    k_float(0, 2) *= static_cast<float>(seam_work_aspect_);
    k_float(1, 1) *= static_cast<float>(seam_work_aspect_);
    k_float(1, 2) *= static_cast<float>(seam_work_aspect_);

    corners[i] =
        warper->warp(seam_est_imgs_[i], k_float, cameras_[i].R, interp_flags_,
                     cv::BORDER_REFLECT, images_warped[i]);
    sizes[i] = images_warped[i].size();

    warper->warp(masks[i], k_float, cameras_[i].R, cv::INTER_NEAREST,
                 cv::BORDER_CONSTANT, masks_warped[i]);
  }

  warp_timer.Report("Warping images");

  // Compensate exposure before finding seams
  exposure_comp_->feed(corners, images_warped, masks_warped);
  for (size_t i = 0; i < imgs_.size(); ++i) {
    exposure_comp_->apply(static_cast<int>(i), corners[i], images_warped[i],
                          masks_warped[i]);
  }

  // Find seams
  std::vector<cv::UMat> images_warped_f(imgs_.size());
  for (size_t i = 0; i < imgs_.size(); ++i) {
    images_warped[i].convertTo(images_warped_f[i], CV_32F);
  }
  seam_finder_->find(images_warped_f, corners, masks_warped);

  // Release unused memory
  seam_est_imgs_.clear();
  images_warped.clear();
  images_warped_f.clear();
  masks.clear();

  spdlog::info("Compositing...");
  auto compositing_total_timer = Timer();

  cv::UMat img_warped;
  cv::UMat dilated_mask;
  cv::UMat seam_mask;
  cv::UMat mask;
  cv::UMat mask_warped;

  // double compose_seam_aspect = 1;
  double compose_work_aspect = 1;
  bool is_blender_prepared = false;

  double compose_scale = 1;
  bool is_compose_scale_set = false;

  std::vector<cv::detail::CameraParams> cameras_scaled(cameras_);

  cv::UMat full_img;
  cv::UMat img;
  for (size_t img_idx = 0; img_idx < imgs_.size(); ++img_idx) {
    spdlog::trace("Compositing image #{}", indices_[img_idx] + 1);
    auto compositing_timer = Timer();

    // Read image and resize it if necessary
    full_img = imgs_[img_idx];
    if (!is_compose_scale_set) {
      auto compose_scale_timer = Timer();
      is_compose_scale_set = true;

      // Compute relative scales
      // compose_seam_aspect = compose_scale / seam_scale_;
      compose_work_aspect = compose_scale / work_scale_;

      // Update warped image scale
      auto warp_scale =
          static_cast<float>(warped_image_scale_ * compose_work_aspect);
      warper = warper_creater_->create(warp_scale);

      // Update corners and sizes
      for (size_t i = 0; i < imgs_.size(); ++i) {
        // Update intrinsics
        cameras_scaled[i].ppx *= compose_work_aspect;
        cameras_scaled[i].ppy *= compose_work_aspect;
        cameras_scaled[i].focal *= compose_work_aspect;

        // Update corner and size
        cv::Size full_size = full_img_sizes_[i];

        cv::Mat k_float;
        cameras_scaled[i].K().convertTo(k_float, CV_32F);
        cv::Rect roi = warper->warpRoi(full_size, k_float, cameras_scaled[i].R);
        corners[i] = roi.tl();
        sizes[i] = roi.size();
      }
      compose_scale_timer.Report(" compose scale time");
    }
    if (std::abs(compose_scale - 1) > 1e-1) {
      auto resize_timer = Timer();

      resize(full_img, img, cv::Size(), compose_scale, compose_scale,
             cv::INTER_LINEAR_EXACT);
      resize_timer.Report(" resize time");
    } else {
      img = full_img;
    }
    full_img.release();
    cv::Size img_size = img.size();

    cv::Mat k_float;
    cameras_scaled[img_idx].K().convertTo(k_float, CV_32F);

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

    if (!is_blender_prepared) {
      blender_->prepare(corners, sizes);
      is_blender_prepared = true;
    }
    timer.Report(" other2");

    // Blend the current image
    blender_->feed(img_warped, mask_warped, corners[img_idx]);
    timer.Report(" feed time");

    compositing_timer.Report("Compositing ## time");
  }

  auto blend_timer = Timer();

  cv::UMat result;
  blender_->blend(result, result_mask_);
  blend_timer.Report(" blend time");

  compositing_total_timer.Report("Compositing");

  pano.assign(result);

  return Status::OK;
}

Stitcher::Status Stitcher::Stitch(cv::InputArrayOfArrays images,
                                  cv::OutputArray pano) {
  return Stitch(images, cv::noArray(), pano);
}

Stitcher::Status Stitcher::Stitch(cv::InputArrayOfArrays images,
                                  cv::InputArrayOfArrays masks,
                                  cv::OutputArray pano) {
  Status status = EstimateTransform(images, masks);
  if (status != Status::OK) {
    return status;
  }
  return ComposePanorama(pano);
}

Stitcher::Status Stitcher::MatchImages() {
  if (static_cast<int>(imgs_.size()) < 2) {
    spdlog::error("Need more images");
    return Status::ERR_NEED_MORE_IMGS;
  }

  work_scale_ = 1;
  seam_work_aspect_ = 1;
  seam_scale_ = 1;
  bool is_work_scale_set = false;
  bool is_seam_scale_set = false;
  features_.resize(imgs_.size());
  seam_est_imgs_.resize(imgs_.size());
  full_img_sizes_.resize(imgs_.size());

  spdlog::info("Finding features...");
  auto timer = Timer();

  std::vector<cv::UMat> feature_find_imgs(imgs_.size());
  std::vector<cv::UMat> feature_find_masks(masks_.size());

  for (size_t i = 0; i < imgs_.size(); ++i) {
    full_img_sizes_[i] = imgs_[i].size();
    if (registr_resol_ < 0) {
      feature_find_imgs[i] = imgs_[i];
      work_scale_ = 1;
      is_work_scale_set = true;
    } else {
      if (!is_work_scale_set) {
        work_scale_ = std::min(
            1.0, std::sqrt(registr_resol_ * 1e6 / full_img_sizes_[i].area()));
        is_work_scale_set = true;
      }
      resize(imgs_[i], feature_find_imgs[i], cv::Size(), work_scale_,
             work_scale_, cv::INTER_LINEAR_EXACT);
    }
    if (!is_seam_scale_set) {
      seam_scale_ = std::min(
          1.0, std::sqrt(seam_est_resol_ * 1e6 / full_img_sizes_[i].area()));
      seam_work_aspect_ = seam_scale_ / work_scale_;
      is_seam_scale_set = true;
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

  spdlog::info("Pairwise matching");

  (*features_matcher_)(features_, pairwise_matches_, matching_mask_);
  features_matcher_->collectGarbage();
  timer.Report("Pairwise matching");

  // Leave only images we are sure are from the same panorama
  indices_ = cv::detail::leaveBiggestComponent(
      features_, pairwise_matches_, static_cast<float>(conf_thresh_));
  std::vector<cv::UMat> seam_est_imgs_subset;
  std::vector<cv::UMat> imgs_subset;
  std::vector<cv::Size> full_img_sizes_subset;
  for (int indice : indices_) {
    imgs_subset.push_back(imgs_[indice]);
    seam_est_imgs_subset.push_back(seam_est_imgs_[indice]);
    full_img_sizes_subset.push_back(full_img_sizes_[indice]);
  }
  seam_est_imgs_ = seam_est_imgs_subset;
  imgs_ = imgs_subset;
  full_img_sizes_ = full_img_sizes_subset;

  if (static_cast<int>(imgs_.size()) < 2) {
    spdlog::error("Need more images");
    return Status::ERR_NEED_MORE_IMGS;
  }

  return Status::OK;
}

Stitcher::Status Stitcher::EstimateCameraParams() {
  // estimate homography in global frame
  if (!(*estimator_)(features_, pairwise_matches_, cameras_)) {
    return Status::ERR_HOMOGRAPHY_EST_FAIL;
  }

  for (auto &camera : cameras_) {
    cv::Mat r_float;
    camera.R.convertTo(r_float, CV_32F);
    camera.R = r_float;
    // LOGLN("Initial intrinsic parameters #" << indices_[i] + 1 << ":\n " <<
    // cameras_[i].K());
  }

  bundle_adjuster_->setConfThresh(conf_thresh_);
  if (!(*bundle_adjuster_)(features_, pairwise_matches_, cameras_)) {
    return Status::ERR_CAMERA_PARAMS_ADJUST_FAIL;
  }

  // Find median focal length and use it as final image scale
  std::vector<double> focals;
  for (auto &camera : cameras_) {
    // LOGLN("Camera #" << indices_[i] + 1 << ":\n" << cameras_[i].K());
    focals.push_back(camera.focal);
  }

  std::sort(focals.begin(), focals.end());
  if (focals.size() % 2 == 1) {
    warped_image_scale_ = static_cast<float>(focals[focals.size() / 2]);
  } else {
    warped_image_scale_ = static_cast<float>(focals[focals.size() / 2 - 1] +
                                             focals[focals.size() / 2]) *
                          0.5f;
  }

  if (do_wave_correct_) {
    std::vector<cv::Mat> rmats;
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
  }

  return Status::OK;
}

Stitcher::Status Stitcher::SetTransform(
    cv::InputArrayOfArrays images,
    const std::vector<cv::detail::CameraParams> &cameras) {
  std::vector<int> component;
  for (int i = 0; i < static_cast<int>(images.total()); i++) {
    component.push_back(i);
  }

  return SetTransform(images, cameras, component);
}

Stitcher::Status Stitcher::SetTransform(
    cv::InputArrayOfArrays images,
    const std::vector<cv::detail::CameraParams> &cameras,
    const std::vector<int> &component) {
  //    CV_Assert(images.size() == cameras.size());

  images.getUMatVector(imgs_);
  masks_.clear();

  if (static_cast<int>(imgs_.size()) < 2) {
    spdlog::error("Need more images");
    return Status::ERR_NEED_MORE_IMGS;
  }

  work_scale_ = 1;
  seam_work_aspect_ = 1;
  seam_scale_ = 1;
  bool is_work_scale_set = false;
  bool is_seam_scale_set = false;
  seam_est_imgs_.resize(imgs_.size());
  full_img_sizes_.resize(imgs_.size());

  for (size_t i = 0; i < imgs_.size(); ++i) {
    full_img_sizes_[i] = imgs_[i].size();
    if (registr_resol_ < 0) {
      work_scale_ = 1;
      is_work_scale_set = true;
    } else {
      if (!is_work_scale_set) {
        work_scale_ = std::min(
            1.0, std::sqrt(registr_resol_ * 1e6 / full_img_sizes_[i].area()));
        is_work_scale_set = true;
      }
    }
    if (!is_seam_scale_set) {
      seam_scale_ = std::min(
          1.0, std::sqrt(seam_est_resol_ * 1e6 / full_img_sizes_[i].area()));
      seam_work_aspect_ = seam_scale_ / work_scale_;
      is_seam_scale_set = true;
    }

    resize(imgs_[i], seam_est_imgs_[i], cv::Size(), seam_scale_, seam_scale_,
           cv::INTER_LINEAR_EXACT);
  }

  features_.clear();
  pairwise_matches_.clear();

  indices_ = component;
  std::vector<cv::UMat> seam_est_imgs_subset;
  std::vector<cv::UMat> imgs_subset;
  std::vector<cv::Size> full_img_sizes_subset;
  for (int indice : indices_) {
    imgs_subset.push_back(imgs_[indice]);
    seam_est_imgs_subset.push_back(seam_est_imgs_[indice]);
    full_img_sizes_subset.push_back(full_img_sizes_[indice]);
  }
  seam_est_imgs_ = seam_est_imgs_subset;
  imgs_ = imgs_subset;
  full_img_sizes_ = full_img_sizes_subset;

  if (static_cast<int>(imgs_.size()) < 2) {
    spdlog::error("Need more images");
    return Status::ERR_NEED_MORE_IMGS;
  }

  cameras_ = cameras;

  std::vector<double> focals;
  for (size_t i = 0; i < cameras.size(); ++i) {
    focals.push_back(cameras_[i].focal);
  }

  std::sort(focals.begin(), focals.end());
  if (focals.size() % 2 == 1) {
    warped_image_scale_ = static_cast<float>(focals[focals.size() / 2]);
  } else {
    warped_image_scale_ = static_cast<float>(focals[focals.size() / 2 - 1] +
                                             focals[focals.size() / 2]) *
                          0.5f;
  }

  return Status::OK;
}

}  // namespace xpano::algorithm::stitcher
