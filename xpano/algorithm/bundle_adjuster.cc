//
//  Contents of the setUpInitialCameraParams function are copied from OpenCV,
//  adding the copyright notice:
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

#include "xpano/algorithm/bundle_adjuster.h"

#include <vector>

#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/stitching/detail/motion_estimators.hpp>

namespace xpano::algorithm {

namespace {
cv::detail::WaveCorrectKind DetectWaveCorrect(
    const std::vector<cv::detail::CameraParams>& cameras) {
  std::vector<cv::Mat> rmats;
  for (size_t i = 0; i < cameras.size(); ++i) {
    rmats.push_back(cameras[i].R.clone());
  }

  return cv::detail::autoDetectWaveCorrectKind(rmats);
}

}  // namespace

void BundleAdjusterRayCustom::setUpInitialCameraParams(
    const std::vector<cv::detail::CameraParams>& cameras) {
  kind_ = DetectWaveCorrect(cameras);

  // Copy of BundleAdjusterRay::setUpInitialCameraParams, cannot call the
  // function since it is private
  cam_params_.create(num_images_ * 4, 1, CV_64F);
  cv::SVD svd;
  for (int i = 0; i < num_images_; ++i) {
    cam_params_.at<double>(i * 4, 0) = cameras[i].focal;

    svd(cameras[i].R, cv::SVD::FULL_UV);
    cv::Mat R = svd.u * svd.vt;
    if (cv::determinant(R) < 0) R *= -1;

    cv::Mat rvec;
    cv::Rodrigues(R, rvec);
    CV_Assert(rvec.type() == CV_32F);
    cam_params_.at<double>(i * 4 + 1, 0) = rvec.at<float>(0, 0);
    cam_params_.at<double>(i * 4 + 2, 0) = rvec.at<float>(1, 0);
    cam_params_.at<double>(i * 4 + 3, 0) = rvec.at<float>(2, 0);
  }
}

cv::detail::WaveCorrectKind BundleAdjusterRayCustom::WaveCorrectionKind()
    const {
  return kind_;
}

}  // namespace xpano::algorithm
