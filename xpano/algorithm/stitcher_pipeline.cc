#include "xpano/algorithm/stitcher_pipeline.h"

#include <algorithm>
#include <atomic>
#include <future>
#include <optional>
#include <string>
#include <vector>

#include <BS_thread_pool.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/stitching.hpp>
#include <spdlog/spdlog.h>

#include "xpano/algorithm/algorithm.h"
#include "xpano/algorithm/image.h"
#include "xpano/constants.h"

namespace xpano::algorithm {
namespace {

std::vector<int> CompressionParameters(const CompressionOptions &options) {
  return {
      cv::IMWRITE_JPEG_QUALITY,     options.jpeg_quality,
      cv::IMWRITE_JPEG_PROGRESSIVE, static_cast<int>(options.jpeg_progressive),
      cv::IMWRITE_JPEG_OPTIMIZE,    static_cast<int>(options.jpeg_optimize),
      cv::IMWRITE_PNG_COMPRESSION,  options.png_compression};
}

}  // namespace

void ProgressMonitor::Reset(ProgressType type, int num_tasks) {
  type_ = type;
  done_ = 0;
  num_tasks_ = num_tasks;
}

void ProgressMonitor::SetTaskType(ProgressType type) { type_ = type; }

void ProgressMonitor::SetNumTasks(int num_tasks) { num_tasks_ = num_tasks; }

ProgressReport ProgressMonitor::Progress() const {
  return {type_, done_, num_tasks_};
}

void ProgressMonitor::NotifyTaskDone() { done_++; }

StitcherPipeline::~StitcherPipeline() { Cancel(); }

void StitcherPipeline::Cancel() {
  if (pool_.get_tasks_total() > 0) {
    pool_.pause();

    spdlog::info("Waiting for running tasks to finish");
    cancel_tasks_ = true;
    pool_.wait_for_tasks();
    cancel_tasks_ = false;

    spdlog::info("Cancelling remaining tasks");
    pool_.cancel_tasks();
    pool_.unpause();

    loading_progress_.Reset(ProgressType::kNone, 0);
  }
}

std::future<StitcherData> StitcherPipeline::RunLoading(
    const std::vector<std::string> &inputs,
    const LoadingOptions &loading_options,
    const MatchingOptions &matching_options) {
  return pool_.submit([this, loading_options, matching_options, inputs]() {
    auto images = RunLoadingPipeline(inputs, loading_options);
    return RunMatchingPipeline(images, matching_options);
  });
}

std::future<StitchingResult> StitcherPipeline::RunStitching(
    const StitcherData &data, const StitchingOptions &options) {
  std::vector<cv::Mat> imgs;
  auto pano = data.panos[options.pano_id];

  return pool_.submit([pano, &images = data.images, options, this]() {
    int num_tasks = static_cast<int>(pano.ids.size()) + 1 +
                    static_cast<int>(options.export_path.has_value());
    loading_progress_.Reset(ProgressType::kLoadingImages, num_tasks);
    std::vector<cv::Mat> imgs;
    for (int img_id : pano.ids) {
      if (options.export_path) {
        imgs.push_back(images[img_id].GetFullRes());
      } else {
        imgs.push_back(images[img_id].GetPreview());
      }
      loading_progress_.NotifyTaskDone();
    }

    loading_progress_.SetTaskType(ProgressType::kStitchingPano);
    auto [status, pano] = Stitch(imgs);
    loading_progress_.NotifyTaskDone();

    if (status != cv::Stitcher::OK) {
      return StitchingResult{options.pano_id, status};
    }

    std::optional<std::string> export_path;
    if (options.export_path) {
      if (cv::imwrite(*options.export_path, pano,
                      CompressionParameters(options.compression))) {
        export_path = options.export_path;
      }
      loading_progress_.NotifyTaskDone();
    }

    return StitchingResult{options.pano_id, status, pano, export_path};
  });
}

ProgressReport StitcherPipeline::LoadingProgress() const {
  return loading_progress_.Progress();
}

std::vector<algorithm::Image> StitcherPipeline::RunLoadingPipeline(
    const std::vector<std::string> &inputs, const LoadingOptions &options) {
  int num_tasks = static_cast<int>(inputs.size());
  loading_progress_.Reset(ProgressType::kDetectingKeypoints, num_tasks);
  BS::multi_future<algorithm::Image> loading_future;
  for (const auto &input : inputs) {
    loading_future.push_back(pool_.submit([this, options, input]() {
      Image image(input);
      image.Load(options.preview_longer_side);
      loading_progress_.NotifyTaskDone();
      return image;
    }));
  }

  std::future_status status;
  while ((status = loading_future.wait_for(kTaskCancellationTimeout)) !=
         std::future_status::ready) {
    if (cancel_tasks_) {
      return {};
    }
  }
  auto images = loading_future.get();

  auto num_erased =
      std::erase_if(images, [](const Image &img) { return !img.IsLoaded(); });
  if (num_erased > 0) {
    spdlog::warn("Failed to load {} images", num_erased);
  }
  return images;
}

StitcherData StitcherPipeline::RunMatchingPipeline(
    std::vector<algorithm::Image> images, const MatchingOptions &options) {
  if (images.empty()) {
    return {};
  }

  int num_images = static_cast<int>(images.size());
  int num_neighbors =
      std::min(options.neighborhood_search_size, num_images - 1);
  int num_tasks =
      1 +                                             // FindPanos
      (num_images - num_neighbors) * num_neighbors +  // full n-tuples
      ((num_neighbors - 1) * num_neighbors) / 2;      // non-full (j - i < 0)

  loading_progress_.Reset(ProgressType::kMatchingImages, num_tasks);
  BS::multi_future<Match> matches_future;
  for (int j = 0; j < images.size(); j++) {
    for (int i = std::max(0, j - num_neighbors); i < j; i++) {
      matches_future.push_back(
          pool_.submit([this, i, j, left = images[i], right = images[j]]() {
            auto match = Match{i, j, MatchImages(left, right)};
            loading_progress_.NotifyTaskDone();
            return match;
          }));
    }
  }

  std::future_status status;
  while ((status = matches_future.wait_for(kTaskCancellationTimeout)) !=
         std::future_status::ready) {
    if (cancel_tasks_) {
      return {};
    }
  }
  auto matches = matches_future.get();

  auto panos = FindPanos(matches, options.match_threshold);
  loading_progress_.NotifyTaskDone();
  return StitcherData{images, matches, panos};
}

}  // namespace xpano::algorithm
