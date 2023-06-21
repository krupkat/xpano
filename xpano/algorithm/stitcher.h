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

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/stitching.hpp>

namespace xpano::algorithm::stitcher {

class Stitcher {
 public:
  static constexpr double ORIG_RESOL = -1.0;

  using Status = cv::Stitcher::Status;
  using Mode = cv::Stitcher::Mode;

  static cv::Ptr<Stitcher> create(Mode mode = Stitcher::Mode::PANORAMA);

  double registrationResol() const { return registr_resol_; }
  void setRegistrationResol(double resol_mpx) { registr_resol_ = resol_mpx; }

  double seamEstimationResol() const { return seam_est_resol_; }
  void setSeamEstimationResol(double resol_mpx) { seam_est_resol_ = resol_mpx; }

  double compositingResol() const { return compose_resol_; }
  void setCompositingResol(double resol_mpx) { compose_resol_ = resol_mpx; }

  double panoConfidenceThresh() const { return conf_thresh_; }
  void setPanoConfidenceThresh(double conf_thresh) {
    conf_thresh_ = conf_thresh;
  }

  bool waveCorrection() const { return do_wave_correct_; }
  void setWaveCorrection(bool flag) { do_wave_correct_ = flag; }

  cv::InterpolationFlags InterpolationFlags() const { return interp_flags_; }
  void setInterpolationFlags(cv::InterpolationFlags interp_flags) {
    interp_flags_ = interp_flags;
  }

  cv::detail::WaveCorrectKind waveCorrectKind() const {
    return wave_correct_kind_;
  }
  void setWaveCorrectKind(cv::detail::WaveCorrectKind kind) {
    wave_correct_kind_ = kind;
  }

  cv::Ptr<cv::Feature2D> featuresFinder() { return features_finder_; }
  cv::Ptr<cv::Feature2D> featuresFinder() const { return features_finder_; }
  void setFeaturesFinder(cv::Ptr<cv::Feature2D> features_finder) {
    features_finder_ = features_finder;
  }

  cv::Ptr<cv::detail::FeaturesMatcher> featuresMatcher() {
    return features_matcher_;
  }
  cv::Ptr<cv::detail::FeaturesMatcher> featuresMatcher() const {
    return features_matcher_;
  }
  void setFeaturesMatcher(
      cv::Ptr<cv::detail::FeaturesMatcher> features_matcher) {
    features_matcher_ = features_matcher;
  }

  const cv::UMat &matchingMask() const { return matching_mask_; }
  void setMatchingMask(const cv::UMat &mask) {
    CV_Assert(mask.type() == CV_8U && mask.cols == mask.rows);
    matching_mask_ = mask.clone();
  }

  cv::Ptr<cv::detail::BundleAdjusterBase> bundleAdjuster() {
    return bundle_adjuster_;
  }
  const cv::Ptr<cv::detail::BundleAdjusterBase> bundleAdjuster() const {
    return bundle_adjuster_;
  }
  void setBundleAdjuster(
      cv::Ptr<cv::detail::BundleAdjusterBase> bundle_adjuster) {
    bundle_adjuster_ = bundle_adjuster;
  }

  cv::Ptr<cv::detail::Estimator> estimator() { return estimator_; }
  const cv::Ptr<cv::detail::Estimator> estimator() const { return estimator_; }
  void setEstimator(cv::Ptr<cv::detail::Estimator> estimator) {
    estimator_ = estimator;
  }

  cv::Ptr<cv::WarperCreator> warper() { return warper_; }
  const cv::Ptr<cv::WarperCreator> warper() const { return warper_; }
  void setWarper(cv::Ptr<cv::WarperCreator> creator) { warper_ = creator; }

  cv::Ptr<cv::detail::ExposureCompensator> exposureCompensator() {
    return exposure_comp_;
  }
  const cv::Ptr<cv::detail::ExposureCompensator> exposureCompensator() const {
    return exposure_comp_;
  }
  void setExposureCompensator(
      cv::Ptr<cv::detail::ExposureCompensator> exposure_comp) {
    exposure_comp_ = exposure_comp;
  }

  cv::Ptr<cv::detail::SeamFinder> seamFinder() { return seam_finder_; }
  const cv::Ptr<cv::detail::SeamFinder> seamFinder() const {
    return seam_finder_;
  }
  void setSeamFinder(cv::Ptr<cv::detail::SeamFinder> seam_finder) {
    seam_finder_ = seam_finder;
  }

  cv::Ptr<cv::detail::Blender> blender() { return blender_; }
  const cv::Ptr<cv::detail::Blender> blender() const { return blender_; }
  void setBlender(cv::Ptr<cv::detail::Blender> b) { blender_ = b; }

  Status estimateTransform(cv::InputArrayOfArrays images,
                           cv::InputArrayOfArrays masks = cv::noArray());

  Status setTransform(cv::InputArrayOfArrays images,
                      const std::vector<cv::detail::CameraParams> &cameras,
                      const std::vector<int> &component);
  Status setTransform(cv::InputArrayOfArrays images,
                      const std::vector<cv::detail::CameraParams> &cameras);

  Status composePanorama(cv::OutputArray pano);

  Status composePanorama(cv::InputArrayOfArrays images, cv::OutputArray pano);

  Status stitch(cv::InputArrayOfArrays images, cv::OutputArray pano);

  Status stitch(cv::InputArrayOfArrays images, cv::InputArrayOfArrays masks,
                cv::OutputArray pano);

  std::vector<int> component() const { return indices_; }
  std::vector<cv::detail::CameraParams> cameras() const { return cameras_; }
  double workScale() const { return work_scale_; }

  cv::UMat resultMask() const { return result_mask_; }

 private:
  Status matchImages();
  Status estimateCameraParams();

  double registr_resol_;
  double seam_est_resol_;
  double compose_resol_;
  double conf_thresh_;
  cv::InterpolationFlags interp_flags_;
  cv::Ptr<cv::Feature2D> features_finder_;
  cv::Ptr<cv::detail::FeaturesMatcher> features_matcher_;
  cv::UMat matching_mask_;
  cv::Ptr<cv::detail::BundleAdjusterBase> bundle_adjuster_;
  cv::Ptr<cv::detail::Estimator> estimator_;
  bool do_wave_correct_;
  cv::detail::WaveCorrectKind wave_correct_kind_;
  cv::Ptr<cv::WarperCreator> warper_;
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
  double work_scale_;
  double seam_scale_;
  double seam_work_aspect_;
  double warped_image_scale_;
};

}  // namespace xpano::algorithm::stitcher
