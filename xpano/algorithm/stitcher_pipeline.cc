#include "algorithm/stitcher_pipeline.h"

#include <atomic>
#include <cstddef>
#include <future>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <BS_thread_pool.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <spdlog/spdlog.h>

#include "algorithm/algorithm.h"
#include "algorithm/image.h"

namespace xpano::algorithm {
namespace {

std::vector<int> CompressionParameters(const CompressionOptions &options) {
  return {cv::IMWRITE_JPEG_QUALITY,     options.jpeg_quality,
          cv::IMWRITE_JPEG_PROGRESSIVE, options.jpeg_progressive,
          cv::IMWRITE_JPEG_OPTIMIZE,    options.jpeg_optimize,
          cv::IMWRITE_PNG_COMPRESSION,  options.png_compression};
}

}  // namespace

void ProgressMonitor::Monitor(ProgressType type, int num_tasks) {
  type_ = type;
  done_ = 0;
  num_tasks_ = num_tasks;
}

void ProgressMonitor::Monitor(ProgressType type) { type_ = type; }

ProgressReport ProgressMonitor::Progress() const {
  return {type_, done_, num_tasks_};
}

void ProgressMonitor::NotifyTaskDone() { done_++; }

std::future<StitcherData> StitcherPipeline::RunLoading(
    const std::vector<std::string> &inputs, const LoadingOptions &options) {
  pool_.wait_for_tasks();
  options_ = options;

  return pool_.submit([this, inputs]() { return RunLoadingPipeline(inputs); });
}

std::future<StitchingResult> StitcherPipeline::RunStitching(
    const StitcherData &data, const StitchingOptions &options) {
  std::vector<cv::Mat> imgs;
  auto pano = data.panos[options.pano_id];

  return pool_.submit([pano, &images = data.images, options, this]() {
    int num_tasks =
        static_cast<int>(pano.ids.size()) + 1 + options.export_path.has_value();
    loading_progress_.Monitor(ProgressType::kLoadingImages, num_tasks);
    std::vector<cv::Mat> imgs;
    for (int img_id : pano.ids) {
      if (options.export_path) {
        imgs.push_back(images[img_id].GetFullRes());
      } else {
        imgs.push_back(images[img_id].GetPreview());
      }
      loading_progress_.NotifyTaskDone();
    }

    loading_progress_.Monitor(ProgressType::kStitchingPano);
    auto pano = Stitch(imgs);
    loading_progress_.NotifyTaskDone();

    std::optional<std::string> export_path;
    if (options.export_path) {
      if (pano) {
        if (cv::imwrite(*options.export_path, *pano,
                        CompressionParameters(options.compression))) {
          export_path = options.export_path;
        }
      }
      loading_progress_.NotifyTaskDone();
    }

    return StitchingResult{options.pano_id, pano, export_path};
  });
}

ProgressReport StitcherPipeline::LoadingProgress() const {
  return loading_progress_.Progress();
}

StitcherData StitcherPipeline::RunLoadingPipeline(
    const std::vector<std::string> &inputs) {
  auto images = std::vector<Image>{inputs.begin(), inputs.end()};

  int num_tasks = static_cast<int>(images.size()) * 2;
  loading_progress_.Monitor(ProgressType::kDetectingKeypoints, num_tasks);
  pool_
      .parallelize_loop(0, images.size(),
                        [this, &images](size_t start, size_t end) {
                          for (size_t i = start; i < end; ++i) {
                            images[i].Load();
                            loading_progress_.NotifyTaskDone();
                          }
                        })
      .wait();

  loading_progress_.Monitor(ProgressType::kMatchingImages);
  BS::multi_future<Match> matches_future;
  for (int i = 1; i < images.size(); i++) {
    matches_future.push_back(pool_.submit([this, i, &images]() {
      auto match = Match{i - 1, i, MatchImages(images[i - 1], images[i])};
      loading_progress_.NotifyTaskDone();
      return match;
    }));
  }

  std::vector<algorithm::Match> matches;
  try {
    matches = matches_future.get();
  } catch (const std::exception &e) {
    spdlog::error("{} ", e.what());
    return {};
  }

  auto panos = FindPanos(matches);
  loading_progress_.NotifyTaskDone();

  return StitcherData{images, matches, panos};
}

}  // namespace xpano::algorithm
