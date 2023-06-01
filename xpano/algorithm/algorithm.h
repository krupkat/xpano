// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Vaibhav Sharma
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/stitching.hpp>

#include "xpano/algorithm/image.h"
#include "xpano/algorithm/options.h"
#include "xpano/constants.h"
#include "xpano/utils/rect.h"
#include "xpano/utils/threadpool.h"

namespace xpano::algorithm {

struct Pano {
  std::vector<int> ids;
  bool exported = false;
};

struct Match {
  int id1;
  int id2;
  std::vector<cv::DMatch> matches;
};

Pano SinglePano(int size);

std::vector<cv::DMatch> MatchImages(const Image& img1, const Image& img2,
                                    float match_conf);

std::vector<Pano> FindPanos(const std::vector<Match>& matches,
                            int match_threshold);

struct StitchResult {
  cv::Stitcher::Status status;
  cv::Mat pano;
  cv::Mat mask;
};

StitchResult Stitch(const std::vector<cv::Mat>& images, StitchOptions options,
                    bool return_pano_mask, utils::mt::Threadpool* threadpool);

std::string ToString(cv::Stitcher::Status& status);

std::optional<utils::RectRRf> FindLargestCrop(const cv::Mat& mask);

cv::Mat Inpaint(const cv::Mat& pano, const cv::Mat& mask,
                InpaintingOptions options);

}  // namespace xpano::algorithm
