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

#include <spdlog/spdlog.h>

namespace xpano::algorithm::stitcher {

cv::Ptr<Stitcher> Stitcher::create(Mode mode) {
  cv::Ptr<Stitcher> stitcher = cv::makePtr<Stitcher>();

  stitcher->setRegistrationResol(0.6);
  stitcher->setSeamEstimationResol(0.1);
  stitcher->setCompositingResol(ORIG_RESOL);
  stitcher->setPanoConfidenceThresh(1);
  stitcher->setSeamFinder(cv::makePtr<cv::detail::GraphCutSeamFinder>(
      cv::detail::GraphCutSeamFinderBase::COST_COLOR));
  stitcher->setBlender(cv::makePtr<cv::detail::MultiBandBlender>(false));
  stitcher->setFeaturesFinder(cv::ORB::create());
  stitcher->setInterpolationFlags(cv::INTER_LINEAR);

  stitcher->work_scale_ = 1;
  stitcher->seam_scale_ = 1;
  stitcher->seam_work_aspect_ = 1;
  stitcher->warped_image_scale_ = 1;

  switch (mode) {
    case Mode::PANORAMA:  // PANORAMA is the default
      // mostly already setup
      stitcher->setEstimator(
          cv::makePtr<cv::detail::HomographyBasedEstimator>());
      stitcher->setWaveCorrection(true);
      stitcher->setWaveCorrectKind(cv::detail::WAVE_CORRECT_HORIZ);
      stitcher->setFeaturesMatcher(
          cv::makePtr<cv::detail::BestOf2NearestMatcher>(false));
      stitcher->setBundleAdjuster(cv::makePtr<cv::detail::BundleAdjusterRay>());
      stitcher->setWarper(cv::makePtr<cv::SphericalWarper>());
      stitcher->setExposureCompensator(
          cv::makePtr<cv::detail::BlocksGainCompensator>());
      break;

    case Mode::SCANS:
      stitcher->setEstimator(cv::makePtr<cv::detail::AffineBasedEstimator>());
      stitcher->setWaveCorrection(false);
      stitcher->setFeaturesMatcher(
          cv::makePtr<cv::detail::AffineBestOf2NearestMatcher>(false, false));
      stitcher->setBundleAdjuster(
          cv::makePtr<cv::detail::BundleAdjusterAffinePartial>());
      stitcher->setWarper(cv::makePtr<cv::AffineWarper>());
      stitcher->setExposureCompensator(
          cv::makePtr<cv::detail::NoExposureCompensator>());
      break;

    default:
      CV_Error(cv::Error::StsBadArg,
               "Invalid stitching mode. Must be one of Stitcher::Mode");
      break;
  }

  return stitcher;
}

Stitcher::Status Stitcher::estimateTransform(cv::InputArrayOfArrays images,
                                             cv::InputArrayOfArrays masks) {
  images.getUMatVector(imgs_);
  masks.getUMatVector(masks_);

  Status status;

  if ((status = matchImages()) != Status::OK) return status;

  if ((status = estimateCameraParams()) != Status::OK) return status;

  return Status::OK;
}

Stitcher::Status Stitcher::composePanorama(cv::OutputArray pano) {
  return composePanorama(std::vector<cv::UMat>(), pano);
}

Stitcher::Status Stitcher::composePanorama(cv::InputArrayOfArrays images,
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

    for (size_t i = 0; i < indices_.size(); ++i) {
      imgs_subset.push_back(imgs_[indices_[i]]);
      seam_est_imgs_subset.push_back(seam_est_imgs_[indices_[i]]);
    }

    seam_est_imgs_ = seam_est_imgs_subset;
    imgs_ = imgs_subset;
  }

  cv::UMat pano_;

  int64 t = cv::getTickCount();

  std::vector<cv::Point> corners(imgs_.size());
  std::vector<cv::UMat> masks_warped(imgs_.size());
  std::vector<cv::UMat> images_warped(imgs_.size());
  std::vector<cv::Size> sizes(imgs_.size());
  std::vector<cv::UMat> masks(imgs_.size());

  // Prepare image masks
  for (size_t i = 0; i < imgs_.size(); ++i) {
    masks[i].create(seam_est_imgs_[i].size(), CV_8U);
    masks[i].setTo(cv::Scalar::all(255));
  }

  // Warp images and their masks
  cv::Ptr<cv::detail::RotationWarper> w =
      warper_->create(float(warped_image_scale_ * seam_work_aspect_));
  for (size_t i = 0; i < imgs_.size(); ++i) {
    cv::Mat_<float> K;
    cameras_[i].K().convertTo(K, CV_32F);
    K(0, 0) *= (float)seam_work_aspect_;
    K(0, 2) *= (float)seam_work_aspect_;
    K(1, 1) *= (float)seam_work_aspect_;
    K(1, 2) *= (float)seam_work_aspect_;

    corners[i] = w->warp(seam_est_imgs_[i], K, cameras_[i].R, interp_flags_,
                         cv::BORDER_REFLECT, images_warped[i]);
    sizes[i] = images_warped[i].size();

    w->warp(masks[i], K, cameras_[i].R, cv::INTER_NEAREST, cv::BORDER_CONSTANT,
            masks_warped[i]);
  }

  spdlog::trace("Warping images, time: {} sec",
                (cv::getTickCount() - t) / cv::getTickFrequency());

  // Compensate exposure before finding seams
  exposure_comp_->feed(corners, images_warped, masks_warped);
  for (size_t i = 0; i < imgs_.size(); ++i)
    exposure_comp_->apply(int(i), corners[i], images_warped[i],
                          masks_warped[i]);

  // Find seams
  std::vector<cv::UMat> images_warped_f(imgs_.size());
  for (size_t i = 0; i < imgs_.size(); ++i)
    images_warped[i].convertTo(images_warped_f[i], CV_32F);
  seam_finder_->find(images_warped_f, corners, masks_warped);

  // Release unused memory
  seam_est_imgs_.clear();
  images_warped.clear();
  images_warped_f.clear();
  masks.clear();

  spdlog::info("Compositing...");
  t = cv::getTickCount();

  cv::UMat img_warped, img_warped_s;
  cv::UMat dilated_mask, seam_mask, mask, mask_warped;

  // double compose_seam_aspect = 1;
  double compose_work_aspect = 1;
  bool is_blender_prepared = false;

  double compose_scale = 1;
  bool is_compose_scale_set = false;

  std::vector<cv::detail::CameraParams> cameras_scaled(cameras_);

  cv::UMat full_img, img;
  for (size_t img_idx = 0; img_idx < imgs_.size(); ++img_idx) {
    spdlog::trace("Compositing image #{}", indices_[img_idx] + 1);
    int64 compositing_t = cv::getTickCount();

    // Read image and resize it if necessary
    full_img = imgs_[img_idx];
    if (!is_compose_scale_set) {
      if (compose_resol_ > 0)
        compose_scale = std::min(
            1.0, std::sqrt(compose_resol_ * 1e6 / full_img.size().area()));
      is_compose_scale_set = true;

      // Compute relative scales
      // compose_seam_aspect = compose_scale / seam_scale_;
      compose_work_aspect = compose_scale / work_scale_;

      // Update warped image scale
      float warp_scale =
          static_cast<float>(warped_image_scale_ * compose_work_aspect);
      w = warper_->create(warp_scale);

      // Update corners and sizes
      for (size_t i = 0; i < imgs_.size(); ++i) {
        // Update intrinsics
        cameras_scaled[i].ppx *= compose_work_aspect;
        cameras_scaled[i].ppy *= compose_work_aspect;
        cameras_scaled[i].focal *= compose_work_aspect;

        // Update corner and size
        cv::Size sz = full_img_sizes_[i];
        if (std::abs(compose_scale - 1) > 1e-1) {
          sz.width = cvRound(full_img_sizes_[i].width * compose_scale);
          sz.height = cvRound(full_img_sizes_[i].height * compose_scale);
        }

        cv::Mat K;
        cameras_scaled[i].K().convertTo(K, CV_32F);
        cv::Rect roi = w->warpRoi(sz, K, cameras_scaled[i].R);
        corners[i] = roi.tl();
        sizes[i] = roi.size();
      }
    }
    if (std::abs(compose_scale - 1) > 1e-1) {
      int64 resize_t = cv::getTickCount();

      resize(full_img, img, cv::Size(), compose_scale, compose_scale,
             cv::INTER_LINEAR_EXACT);
      spdlog::trace("  resize time: {} sec",
                    (cv::getTickCount() - resize_t) / cv::getTickFrequency());
    } else
      img = full_img;
    full_img.release();
    cv::Size img_size = img.size();

    spdlog::trace(
        " after resize time: {} sec",
        (cv::getTickCount() - compositing_t) / cv::getTickFrequency());

    cv::Mat K;
    cameras_scaled[img_idx].K().convertTo(K, CV_32F);

    int64 pt = cv::getTickCount();

    // Warp the current image
    w->warp(img, K, cameras_[img_idx].R, interp_flags_, cv::BORDER_REFLECT,
            img_warped);
    spdlog::trace(" warp the current image: {} sec",
                  (cv::getTickCount() - pt) / cv::getTickFrequency());

    pt = cv::getTickCount();

    // Warp the current image mask
    mask.create(img_size, CV_8U);
    mask.setTo(cv::Scalar::all(255));
    w->warp(mask, K, cameras_[img_idx].R, cv::INTER_NEAREST,
            cv::BORDER_CONSTANT, mask_warped);
    spdlog::trace(" warp the current image mask: {} sec",
                  (cv::getTickCount() - pt) / cv::getTickFrequency());

    pt = cv::getTickCount();

    // Compensate exposure
    exposure_comp_->apply((int)img_idx, corners[img_idx], img_warped,
                          mask_warped);
    spdlog::trace(" compensate exposure: {} sec",
                  (cv::getTickCount() - pt) / cv::getTickFrequency());

    pt = cv::getTickCount();

    img_warped.convertTo(img_warped_s, CV_16S);
    img_warped.release();
    img.release();
    mask.release();

    // Make sure seam mask has proper size
    dilate(masks_warped[img_idx], dilated_mask, cv::Mat());
    resize(dilated_mask, seam_mask, mask_warped.size(), 0, 0,
           cv::INTER_LINEAR_EXACT);

    bitwise_and(seam_mask, mask_warped, mask_warped);

    spdlog::trace(" other: {} sec",
                  (cv::getTickCount() - pt) / cv::getTickFrequency());

    pt = cv::getTickCount();

    if (!is_blender_prepared) {
      blender_->prepare(corners, sizes);
      is_blender_prepared = true;
    }

    spdlog::trace(" other2: {} sec",
                  (cv::getTickCount() - pt) / cv::getTickFrequency());

    spdlog::info(" feed...");

    int64 feed_t = cv::getTickCount();

    // Blend the current image
    blender_->feed(img_warped_s, mask_warped, corners[img_idx]);
    spdlog::trace(" feed time: {} sec",
                  (cv::getTickCount() - feed_t) / cv::getTickFrequency());
    spdlog::trace(
        "Compositing ## time: {} sec",
        (cv::getTickCount() - compositing_t) / cv::getTickFrequency());
  }

  int64 blend_t = cv::getTickCount();

  cv::UMat result;
  blender_->blend(result, result_mask_);
  spdlog::trace("blend time: {} sec",
                (cv::getTickCount() - blend_t) / cv::getTickFrequency());

  spdlog::trace("Compositing, time: {} sec",
                (cv::getTickCount() - t) / cv::getTickFrequency());

  // Preliminary result is in CV_16SC3 format, but all values are in [0,255]
  // range, so convert it to avoid user confusing
  result.convertTo(pano, CV_8U);

  return Status::OK;
}

Stitcher::Status Stitcher::stitch(cv::InputArrayOfArrays images,
                                  cv::OutputArray pano) {
  return stitch(images, cv::noArray(), pano);
}

Stitcher::Status Stitcher::stitch(cv::InputArrayOfArrays images,
                                  cv::InputArrayOfArrays masks,
                                  cv::OutputArray pano) {
  Status status = estimateTransform(images, masks);
  if (status != Status::OK) {
    return status;
  }
  return composePanorama(pano);
}

Stitcher::Status Stitcher::matchImages() {
  if ((int)imgs_.size() < 2) {
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
  int64 t = cv::getTickCount();

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
    features_[i].img_idx = (int)i;
    spdlog::debug("Features in image #{}: {}", i + 1,
                  features_[i].keypoints.size());

    cv::resize(imgs_[i], seam_est_imgs_[i], cv::Size(), seam_scale_,
               seam_scale_, cv::INTER_LINEAR_EXACT);
  }

  // find features possibly in parallel
  cv::detail::computeImageFeatures(features_finder_, feature_find_imgs,
                                   features_, feature_find_masks);

  // Do it to save memory
  feature_find_imgs.clear();
  feature_find_masks.clear();

  spdlog::trace("Finding features, time: {} sec",
                (cv::getTickCount() - t) / cv::getTickFrequency());

  spdlog::info("Pairwise matching");

  t = cv::getTickCount();

  (*features_matcher_)(features_, pairwise_matches_, matching_mask_);
  features_matcher_->collectGarbage();
  spdlog::trace("Pairwise matching, time: {} sec",
                (cv::getTickCount() - t) / cv::getTickFrequency());

  // Leave only images we are sure are from the same panorama
  indices_ = cv::detail::leaveBiggestComponent(features_, pairwise_matches_,
                                               (float)conf_thresh_);
  std::vector<cv::UMat> seam_est_imgs_subset;
  std::vector<cv::UMat> imgs_subset;
  std::vector<cv::Size> full_img_sizes_subset;
  for (size_t i = 0; i < indices_.size(); ++i) {
    imgs_subset.push_back(imgs_[indices_[i]]);
    seam_est_imgs_subset.push_back(seam_est_imgs_[indices_[i]]);
    full_img_sizes_subset.push_back(full_img_sizes_[indices_[i]]);
  }
  seam_est_imgs_ = seam_est_imgs_subset;
  imgs_ = imgs_subset;
  full_img_sizes_ = full_img_sizes_subset;

  if ((int)imgs_.size() < 2) {
    spdlog::error("Need more images");
    return Status::ERR_NEED_MORE_IMGS;
  }

  return Status::OK;
}

Stitcher::Status Stitcher::estimateCameraParams() {
  // estimate homography in global frame
  if (!(*estimator_)(features_, pairwise_matches_, cameras_)) {
    return Status::ERR_HOMOGRAPHY_EST_FAIL;
  }

  for (size_t i = 0; i < cameras_.size(); ++i) {
    cv::Mat R;
    cameras_[i].R.convertTo(R, CV_32F);
    cameras_[i].R = R;
    // LOGLN("Initial intrinsic parameters #" << indices_[i] + 1 << ":\n " <<
    // cameras_[i].K());
  }

  bundle_adjuster_->setConfThresh(conf_thresh_);
  if (!(*bundle_adjuster_)(features_, pairwise_matches_, cameras_)) {
    return Status::ERR_CAMERA_PARAMS_ADJUST_FAIL;
  }

  // Find median focal length and use it as final image scale
  std::vector<double> focals;
  for (size_t i = 0; i < cameras_.size(); ++i) {
    // LOGLN("Camera #" << indices_[i] + 1 << ":\n" << cameras_[i].K());
    focals.push_back(cameras_[i].focal);
  }

  std::sort(focals.begin(), focals.end());
  if (focals.size() % 2 == 1)
    warped_image_scale_ = static_cast<float>(focals[focals.size() / 2]);
  else
    warped_image_scale_ = static_cast<float>(focals[focals.size() / 2 - 1] +
                                             focals[focals.size() / 2]) *
                          0.5f;

  if (do_wave_correct_) {
    std::vector<cv::Mat> rmats;
    for (size_t i = 0; i < cameras_.size(); ++i)
      rmats.push_back(cameras_[i].R.clone());
    cv::detail::waveCorrect(rmats, wave_correct_kind_);
    for (size_t i = 0; i < cameras_.size(); ++i) cameras_[i].R = rmats[i];
  }

  return Status::OK;
}

Stitcher::Status Stitcher::setTransform(
    cv::InputArrayOfArrays images,
    const std::vector<cv::detail::CameraParams> &cameras) {
  std::vector<int> component;
  for (int i = 0; i < (int)images.total(); i++) component.push_back(i);

  return setTransform(images, cameras, component);
}

Stitcher::Status Stitcher::setTransform(
    cv::InputArrayOfArrays images,
    const std::vector<cv::detail::CameraParams> &cameras,
    const std::vector<int> &component) {
  //    CV_Assert(images.size() == cameras.size());

  images.getUMatVector(imgs_);
  masks_.clear();

  if ((int)imgs_.size() < 2) {
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
  for (size_t i = 0; i < indices_.size(); ++i) {
    imgs_subset.push_back(imgs_[indices_[i]]);
    seam_est_imgs_subset.push_back(seam_est_imgs_[indices_[i]]);
    full_img_sizes_subset.push_back(full_img_sizes_[indices_[i]]);
  }
  seam_est_imgs_ = seam_est_imgs_subset;
  imgs_ = imgs_subset;
  full_img_sizes_ = full_img_sizes_subset;

  if ((int)imgs_.size() < 2) {
    spdlog::error("Need more images");
    return Status::ERR_NEED_MORE_IMGS;
  }

  cameras_ = cameras;

  std::vector<double> focals;
  for (size_t i = 0; i < cameras.size(); ++i)
    focals.push_back(cameras_[i].focal);

  std::sort(focals.begin(), focals.end());
  if (focals.size() % 2 == 1)
    warped_image_scale_ = static_cast<float>(focals[focals.size() / 2]);
  else
    warped_image_scale_ = static_cast<float>(focals[focals.size() / 2 - 1] +
                                             focals[focals.size() / 2]) *
                          0.5f;

  return Status::OK;
}

}  // namespace xpano::algorithm::stitcher
