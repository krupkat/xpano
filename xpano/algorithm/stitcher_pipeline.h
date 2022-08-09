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

struct StitcherPipelineOptions {
  int image_downsample_factor = 1;
};

struct StitcherData {
  std::vector<Image> images;
  std::vector<algorithm::Match> matches;
  std::vector<algorithm::Pano> panos;
};

enum class ProgressType {
  kNone,
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
                                       const StitcherPipelineOptions &options);
  std::future<std::optional<cv::Mat>> RunStitching(const StitcherData &data,
                                                   int pano_id);
  ProgressReport LoadingProgress() const;

 private:
  StitcherData RunLoadingPipeline(const std::vector<std::string> &inputs);

  ProgressMonitor loading_progress_;
  StitcherPipelineOptions options_;
  BS::thread_pool pool_;
};

}  // namespace xpano::algorithm
