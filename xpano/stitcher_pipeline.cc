#include "stitcher_pipeline.h"

#include <atomic>
#include <future>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <BS_thread_pool.hpp>
#include <opencv2/core.hpp>

#include "algorithm.h"
#include "image.h"

namespace xpano {

void ProgressMonitor::Monitor(int num_tasks) {
  done_ = 0;
  num_tasks_ = num_tasks;
}

float ProgressMonitor::Progress() const {
  if (num_tasks_ == 0) {
    return 0.0f;
  }
  return static_cast<float>(done_) / static_cast<float>(num_tasks_);
}

void ProgressMonitor::NotifyTaskDone() { done_++; }

std::future<StitcherData> StitcherPipeline::RunLoading(
    const std::vector<std::string> &inputs,
    const StitcherPipelineOptions &options) {
  pool_.wait_for_tasks();
  options_ = options;

  return pool_.submit([this, inputs]() { return RunLoadingPipeline(inputs); });
}

std::future<std::optional<cv::Mat>> StitcherPipeline::RunStitching(
    const StitcherData &data, int pano_id) {
  std::vector<cv::Mat> imgs;
  for (int img_id : data.panos[pano_id].ids) {
    imgs.push_back(data.images[img_id].GetImageData());
  }

  return pool_.submit(
      [imgs = std::move(imgs)]() { return algorithm::Stitch(imgs); });
}

float StitcherPipeline::LoadingProgress() const {
  return loading_progress_.Progress();
}

StitcherData StitcherPipeline::RunLoadingPipeline(
    const std::vector<std::string> &inputs) {
  auto images = std::vector<Image>{inputs.begin(), inputs.end()};

  int num_tasks = static_cast<int>(images.size()) * 2;
  loading_progress_.Monitor(num_tasks);
  pool_
      .parallelize_loop(0, images.size(),
                        [this, &images](size_t start, size_t end) {
                          for (size_t i = start; i < end; ++i) {
                            images[i].Load();
                            loading_progress_.NotifyTaskDone();
                          }
                        })
      .wait();

  BS::multi_future<algorithm::Match> matches_future;
  for (int i = 1; i < images.size(); i++) {
    matches_future.f.push_back(pool_.submit([this, i, &images]() {
      auto match = algorithm::Match{
          i - 1, i, algorithm::MatchImages(images[i - 1], images[i])};
      loading_progress_.NotifyTaskDone();
      return match;
    }));
  }

  auto matches = matches_future.get();
  auto panos = algorithm::FindPanos(matches);
  loading_progress_.NotifyTaskDone();

  return StitcherData{images, matches, panos};
}

}  // namespace xpano
