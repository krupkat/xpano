// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Vaibhav Sharma
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <future>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/stitching.hpp>

#include "xpano/algorithm/algorithm.h"
#include "xpano/algorithm/image.h"
#include "xpano/algorithm/progress.h"
#include "xpano/constants.h"
#include "xpano/pipeline/options.h"
#include "xpano/utils/rect.h"
#include "xpano/utils/threadpool.h"

namespace xpano::pipeline {

struct StitchingOptions {
  int pano_id = 0;
  bool full_res = false;
  std::optional<std::filesystem::path> export_path;
  MetadataOptions metadata;
  CompressionOptions compression;
  StitchAlgorithmOptions stitch_algorithm;
};

struct ExportOptions {
  int pano_id = 0;
  std::filesystem::path export_path;
  std::optional<std::filesystem::path> metadata_path;
  CompressionOptions compression;
  std::optional<utils::RectRRf> crop;
};

struct StitcherData {
  std::vector<algorithm::Image> images;
  std::vector<algorithm::Match> matches;
  std::vector<algorithm::Pano> panos;
};

struct InpaintingResult {
  cv::Mat pano;
  int pixels_inpainted;
};

struct StitchingResult {
  int pano_id = 0;
  bool full_res = false;
  cv::Stitcher::Status status;
  std::optional<cv::Mat> pano;
  std::optional<utils::RectRRf> auto_crop;
  std::optional<std::filesystem::path> export_path;
  std::optional<cv::Mat> mask;
};

struct ExportResult {
  int pano_id = 0;
  std::optional<std::filesystem::path> export_path;
};

using ProgressMonitor = algorithm::ProgressMonitor;
using ProgressReport = algorithm::ProgressReport;
using ProgressType = algorithm::ProgressType;

class StitcherPipeline {
 public:
  StitcherPipeline() = default;
  ~StitcherPipeline();

  // reason: some tasks use pointers to members
  StitcherPipeline(const StitcherPipeline &) = delete;
  StitcherPipeline &operator=(const StitcherPipeline &) = delete;
  StitcherPipeline(StitcherPipeline &&) = delete;
  StitcherPipeline &operator=(StitcherPipeline &&) = delete;

  std::future<StitcherData> RunLoading(
      const std::vector<std::filesystem::path> &inputs,
      const LoadingOptions &loading_options,
      const MatchingOptions &matching_options);
  std::future<StitchingResult> RunStitching(const StitcherData &data,
                                            const StitchingOptions &options);

  std::future<ExportResult> RunExport(cv::Mat pano,
                                      const ExportOptions &options);
  std::future<InpaintingResult> RunInpainting(cv::Mat pano, cv::Mat mask,
                                              const InpaintingOptions &options);
  ProgressReport Progress() const;

  void Cancel();

 private:
  std::vector<algorithm::Image> RunLoadingPipeline(
      const std::vector<std::filesystem::path> &inputs,
      const LoadingOptions &loading_options, bool compute_keypoints);
  StitcherData RunMatchingPipeline(std::vector<algorithm::Image> images,
                                   const MatchingOptions &options);
  StitchingResult RunStitchingPipeline(
      const algorithm::Pano &pano, const std::vector<algorithm::Image> &images,
      const StitchingOptions &options);

  ExportResult RunExportPipeline(cv::Mat pano, const ExportOptions &options);

  ProgressMonitor progress_;

  std::atomic<bool> cancel_tasks_ = false;
  utils::mt::Threadpool pool_ = {
      std::max(2U, std::thread::hardware_concurrency())};
};

}  // namespace xpano::pipeline
