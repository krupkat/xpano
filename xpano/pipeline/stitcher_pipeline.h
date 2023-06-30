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
#include <variant>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/stitching.hpp>

#include "xpano/algorithm/algorithm.h"
#include "xpano/algorithm/image.h"
#include "xpano/algorithm/progress.h"
#include "xpano/algorithm/stitcher.h"
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
  algorithm::stitcher::Status status;
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

enum class CancelTraits { kAsync, kSync };
enum class RunTraits { kOwnFuture, kReturnFuture };

template <typename Result>
struct Task {
  Result future;
  std::unique_ptr<ProgressMonitor> progress;
};

class StitcherPipeline {
 public:
  StitcherPipeline() = default;
  ~StitcherPipeline();

  using GenericFuture =
      std::variant<std::monostate, std::future<StitcherData>,
                   std::future<StitchingResult>, std::future<ExportResult>,
                   std::future<InpaintingResult>>;

  // reason: some tasks use pointers to members
  StitcherPipeline(const StitcherPipeline &) = delete;
  StitcherPipeline &operator=(const StitcherPipeline &) = delete;
  StitcherPipeline(StitcherPipeline &&) = delete;
  StitcherPipeline &operator=(StitcherPipeline &&) = delete;

  template <RunTraits run = RunTraits::kOwnFuture>
  auto RunLoading(const std::vector<std::filesystem::path> &inputs,
                  const LoadingOptions &loading_options,
                  const MatchingOptions &matching_options)
      -> std::conditional_t<run == RunTraits::kReturnFuture,
                            Task<std::future<StitcherData>>, void>;

  template <RunTraits run = RunTraits::kOwnFuture>
  auto RunStitching(const StitcherData &data, const StitchingOptions &options)
      -> std::conditional_t<run == RunTraits::kReturnFuture,
                            Task<std::future<StitchingResult>>, void>;

  template <RunTraits run = RunTraits::kOwnFuture>
  auto RunExport(cv::Mat pano, const ExportOptions &options)
      -> std::conditional_t<run == RunTraits::kReturnFuture,
                            Task<std::future<ExportResult>>, void>;

  template <RunTraits run = RunTraits::kOwnFuture>
  auto RunInpainting(cv::Mat pano, cv::Mat mask,
                     const InpaintingOptions &options)
      -> std::conditional_t<run == RunTraits::kReturnFuture,
                            Task<std::future<InpaintingResult>>, void>;

  ProgressReport Progress() const;

  std::optional<Task<GenericFuture>> GetReadyTask();

  template <CancelTraits cancel = CancelTraits::kAsync>
  void Cancel();

  void WaitForTasks();

 private:
  std::vector<algorithm::Image> RunLoadingPipeline(
      const std::vector<std::filesystem::path> &inputs,
      const LoadingOptions &loading_options, bool compute_keypoints,
      ProgressMonitor *progress);
  StitcherData RunMatchingPipeline(std::vector<algorithm::Image> images,
                                   const MatchingOptions &options,
                                   ProgressMonitor *progress);
  StitchingResult RunStitchingPipeline(
      const algorithm::Pano &pano, const std::vector<algorithm::Image> &images,
      const StitchingOptions &options, ProgressMonitor *progress);

  ExportResult RunExportPipeline(cv::Mat pano, const ExportOptions &options,
                                 ProgressMonitor *progress);

  void PushTask(Task<GenericFuture> task);

  utils::mt::Threadpool pool_ = {
      std::max(2U, std::thread::hardware_concurrency())};

  // Use a separate threadpool for multiblend.
  // Reason: multiblend doesn't allow cancelling tasks (calling pool_.purge())
  // without either a deadlock or undefined behavior. Primary reason is that it
  // passes many arguments to its subtasks by reference.
  utils::mt::Threadpool multiblend_pool_ = {
      std::max(2U, std::thread::hardware_concurrency() - 1)};

  std::queue<Task<GenericFuture>> queue_;
};

}  // namespace xpano::pipeline
