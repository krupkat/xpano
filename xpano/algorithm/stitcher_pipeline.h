#pragma once

#include <atomic>
#include <future>
#include <optional>
#include <string>
#include <vector>

#include <BS_thread_pool.hpp>
#include <opencv2/core.hpp>

#include "algorithm/algorithm.h"
#include "algorithm/image.h"

namespace xpano::algorithm {

struct CompressionOptions {
  int jpeg_quality = 95;
  bool jpeg_progressive = false;
  bool jpeg_optimize = false;
  int png_compression = 6;
};

struct LoadingOptions {
  int image_downsample_factor = 1;
};

struct StitchingOptions {
  int pano_id = 0;
  std::optional<std::string> export_path;
  CompressionOptions compression;
};

struct StitcherData {
  std::vector<Image> images;
  std::vector<algorithm::Match> matches;
  std::vector<algorithm::Pano> panos;
};

struct StitchingResult {
  int pano_id = 0;
  std::optional<cv::Mat> pano;
  std::optional<std::string> export_path;
};

enum class ProgressType {
  kNone,
  kLoadingImages,
  kStitchingPano,
  kDetectingKeypoints,
  kMatchingImages
};

struct ProgressReport {
  ProgressType type;
  int tasks_done;
  int num_tasks;
};

class ProgressMonitor {
 public:
  void Monitor(ProgressType type, int num_tasks);
  void Monitor(ProgressType type);
  ProgressReport Progress() const;
  void NotifyTaskDone();

 private:
  std::atomic<ProgressType> type_{ProgressType::kNone};
  std::atomic<int> done_ = 0;
  std::atomic<int> num_tasks_ = 0;
};

class StitcherPipeline {
 public:
  StitcherPipeline() = default;
  std::future<StitcherData> RunLoading(const std::vector<std::string> &inputs,
                                       const LoadingOptions &options);
  std::future<StitchingResult> RunStitching(const StitcherData &data,
                                            const StitchingOptions &options);
  ProgressReport LoadingProgress() const;

 private:
  StitcherData RunLoadingPipeline(const std::vector<std::string> &inputs);

  ProgressMonitor loading_progress_;
  LoadingOptions options_;
  BS::thread_pool pool_;
};

}  // namespace xpano::algorithm
