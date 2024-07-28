// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Vaibhav Sharma
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/stitching.hpp>

#include "xpano/algorithm/image.h"
#include "xpano/algorithm/options.h"
#include "xpano/algorithm/progress.h"
#include "xpano/algorithm/stitcher.h"
#include "xpano/utils/rect.h"
#include "xpano/utils/threadpool.h"

namespace xpano::algorithm {

struct Cameras {
  std::vector<cv::detail::CameraParams> cameras;
  std::vector<int> component;
  WaveCorrectionType wave_correction_user;           // set by user
  cv::detail::WaveCorrectKind wave_correction_auto;  // computed by OpenCV
  stitcher::WarpHelper warp_helper;
};

struct Pano {
  std::vector<int> ids;
  bool exported = false;
  std::optional<utils::RectRRf> crop;
  std::optional<utils::RectRRf> auto_crop;
  std::optional<Cameras> cameras;
  std::optional<Cameras> backup_cameras;
};

struct Match {
  int id1;
  int id2;
  std::vector<cv::DMatch> matches;
  float avg_shift = 0.0f;
};

Pano SinglePano(int size);

Match MatchImages(int img1_id, int img2_id, const Image& img1,
                  const Image& img2, float match_conf);

std::vector<Pano> FindPanos(const std::vector<Match>& matches,
                            int match_threshold, float min_shift);

struct StitchResult {
  stitcher::Status status;
  cv::Mat pano;
  cv::Mat mask;
  Cameras cameras;
};

struct StitchOptions {
  bool return_pano_mask = false;
  utils::mt::Threadpool* threads_for_multiblend = nullptr;
  ProgressMonitor* progress_monitor = nullptr;
};

StitchResult Stitch(const std::vector<cv::Mat>& images,
                    const std::optional<Cameras>& cameras,
                    StitchUserOptions user_options, StitchOptions options);

int StitchTasksCount(int num_images, bool cameras_precomputed);

std::string ToString(stitcher::Status& status);

std::optional<utils::RectRRf> FindLargestCrop(const cv::Mat& mask);

cv::Mat Inpaint(const cv::Mat& pano, const cv::Mat& mask,
                InpaintingOptions options);

Cameras Rotate(const Cameras& cameras, const cv::Mat& rotation_matrix);

}  // namespace xpano::algorithm
