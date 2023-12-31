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

#pragma once

#include <atomic>

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/stitching.hpp>

#include "xpano/algorithm/progress.h"

namespace xpano::algorithm::stitcher {

enum class Status {
  kSuccess,
  kCancelled,
  kErrNeedMoreImgs,
  kErrHomographyEstFail,
  kErrCameraParamsAdjustFail
};

struct WarpHelper {
  double work_scale;
  cv::Rect dst_roi;
  cv::Ptr<cv::detail::RotationWarper> warper;
};

class Stitcher {
 public:
  using Mode = cv::Stitcher::Mode;

  static cv::Ptr<Stitcher> Create(Mode mode = Stitcher::Mode::PANORAMA);

  [[nodiscard]] double RegistrationResol() const { return registr_resol_; }
  void SetRegistrationResol(double resol_mpx) { registr_resol_ = resol_mpx; }

  [[nodiscard]] double SeamEstimationResol() const { return seam_est_resol_; }
  void SetSeamEstimationResol(double resol_mpx) { seam_est_resol_ = resol_mpx; }

  [[nodiscard]] double PanoConfidenceThresh() const { return conf_thresh_; }
  void SetPanoConfidenceThresh(double conf_thresh) {
    conf_thresh_ = conf_thresh;
  }

  [[nodiscard]] bool WaveCorrection() const { return do_wave_correct_; }
  void SetWaveCorrection(bool flag) { do_wave_correct_ = flag; }

  [[nodiscard]] cv::InterpolationFlags InterpolationFlags() const {
    return interp_flags_;
  }
  void SetInterpolationFlags(cv::InterpolationFlags interp_flags) {
    interp_flags_ = interp_flags;
  }

  [[nodiscard]] cv::detail::WaveCorrectKind WaveCorrectKind() const {
    return wave_correct_kind_;
  }
  void SetWaveCorrectKind(cv::detail::WaveCorrectKind kind) {
    wave_correct_kind_ = kind;
  }

  cv::Ptr<cv::Feature2D> FeaturesFinder() { return features_finder_; }
  [[nodiscard]] cv::Ptr<cv::Feature2D> FeaturesFinder() const {
    return features_finder_;
  }
  void SetFeaturesFinder(const cv::Ptr<cv::Feature2D>& features_finder) {
    features_finder_ = features_finder;
  }

  cv::Ptr<cv::detail::FeaturesMatcher> FeaturesMatcher() {
    return features_matcher_;
  }
  [[nodiscard]] cv::Ptr<cv::detail::FeaturesMatcher> FeaturesMatcher() const {
    return features_matcher_;
  }
  void SetFeaturesMatcher(
      const cv::Ptr<cv::detail::FeaturesMatcher>& features_matcher) {
    features_matcher_ = features_matcher;
  }

  [[nodiscard]] const cv::UMat& MatchingMask() const { return matching_mask_; }
  void SetMatchingMask(const cv::UMat& mask) {
    CV_Assert(mask.type() == CV_8U && mask.cols == mask.rows);
    matching_mask_ = mask.clone();
  }

  cv::Ptr<cv::detail::BundleAdjusterBase> BundleAdjuster() {
    return bundle_adjuster_;
  }
  [[nodiscard]] cv::Ptr<cv::detail::BundleAdjusterBase> BundleAdjuster() const {
    return bundle_adjuster_;
  }
  void SetBundleAdjuster(
      const cv::Ptr<cv::detail::BundleAdjusterBase>& bundle_adjuster) {
    bundle_adjuster_ = bundle_adjuster;
  }

  cv::Ptr<cv::detail::Estimator> Estimator() { return estimator_; }
  [[nodiscard]] cv::Ptr<cv::detail::Estimator> Estimator() const {
    return estimator_;
  }
  void SetEstimator(const cv::Ptr<cv::detail::Estimator>& estimator) {
    estimator_ = estimator;
  }

  cv::Ptr<cv::WarperCreator> Warper() { return warper_creater_; }
  [[nodiscard]] cv::Ptr<cv::WarperCreator> Warper() const {
    return warper_creater_;
  }
  void SetWarper(const cv::Ptr<cv::WarperCreator>& creator) {
    warper_creater_ = creator;
  }

  cv::Ptr<cv::detail::ExposureCompensator> ExposureCompensator() {
    return exposure_comp_;
  }
  [[nodiscard]] cv::Ptr<cv::detail::ExposureCompensator> ExposureCompensator()
      const {
    return exposure_comp_;
  }
  void SetExposureCompensator(
      const cv::Ptr<cv::detail::ExposureCompensator>& exposure_comp) {
    exposure_comp_ = exposure_comp;
  }

  cv::Ptr<cv::detail::SeamFinder> SeamFinder() { return seam_finder_; }
  [[nodiscard]] cv::Ptr<cv::detail::SeamFinder> SeamFinder() const {
    return seam_finder_;
  }
  void SetSeamFinder(const cv::Ptr<cv::detail::SeamFinder>& seam_finder) {
    seam_finder_ = seam_finder;
  }

  cv::Ptr<cv::detail::Blender> Blender() { return blender_; }
  [[nodiscard]] cv::Ptr<cv::detail::Blender> Blender() const {
    return blender_;
  }
  void SetBlender(const cv::Ptr<cv::detail::Blender>& blender) {
    blender_ = blender;
  }

  Status EstimateTransform(cv::InputArrayOfArrays images,
                           cv::InputArrayOfArrays masks = cv::noArray());

  Status SetTransform(cv::InputArrayOfArrays images,
                      const std::vector<cv::detail::CameraParams>& cameras,
                      const std::vector<int>& component);

  Status SetTransform(cv::InputArrayOfArrays images,
                      const std::vector<cv::detail::CameraParams>& cameras);

  Status ComposePanorama(cv::OutputArray pano);

  Status Stitch(cv::InputArrayOfArrays images, cv::OutputArray pano);

  Status Stitch(cv::InputArrayOfArrays images, cv::InputArrayOfArrays masks,
                cv::OutputArray pano);

  [[nodiscard]] std::vector<int> Component() const { return indices_; }
  [[nodiscard]] std::vector<cv::detail::CameraParams> Cameras() const {
    return cameras_;
  }
  [[nodiscard]] double WorkScale() const { return work_scale_; }

  [[nodiscard]] cv::UMat ResultMask() const { return result_mask_; }

  void SetProgressMonitor(ProgressMonitor* monitor) { monitor_ = monitor; }

  [[nodiscard]] WarpHelper GetWarpHelper() const { return warp_helper_; }

 private:
  Status MatchImages();
  Status EstimateCameraParams();
  Status EstimateSeams(std::vector<cv::UMat>* seams);

  [[nodiscard]] bool Cancelled() const;
  void NextTask(algorithm::ProgressType task);
  void EndMonitoring();

  double registr_resol_;
  double seam_est_resol_;
  double conf_thresh_;

  cv::InterpolationFlags interp_flags_;
  cv::Ptr<cv::Feature2D> features_finder_;
  cv::Ptr<cv::detail::FeaturesMatcher> features_matcher_;
  cv::UMat matching_mask_;
  cv::Ptr<cv::detail::BundleAdjusterBase> bundle_adjuster_;
  cv::Ptr<cv::detail::Estimator> estimator_;
  bool do_wave_correct_;
  cv::detail::WaveCorrectKind wave_correct_kind_;
  cv::Ptr<cv::WarperCreator> warper_creater_;
  cv::Ptr<cv::detail::ExposureCompensator> exposure_comp_;
  cv::Ptr<cv::detail::SeamFinder> seam_finder_;
  cv::Ptr<cv::detail::Blender> blender_;

  std::vector<cv::UMat> imgs_;
  std::vector<cv::UMat> masks_;
  std::vector<cv::Size> full_img_sizes_;
  std::vector<cv::detail::ImageFeatures> features_;
  std::vector<cv::detail::MatchesInfo> pairwise_matches_;
  std::vector<cv::UMat> seam_est_imgs_;
  std::vector<int> indices_;
  std::vector<cv::detail::CameraParams> cameras_;
  cv::UMat result_mask_;

  double work_scale_ = 1.0;
  double seam_scale_ = 1.0;
  double seam_work_aspect_ = 1.0;
  double warped_image_scale_ = 1.0;

  ProgressMonitor* monitor_ = nullptr;
  WarpHelper warp_helper_ = {};
};

}  // namespace xpano::algorithm::stitcher
