// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Vaibhav Sharma
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/pipeline/stitcher_pipeline.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <future>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/stitching.hpp>
#include <spdlog/spdlog.h>

#include "xpano/algorithm/algorithm.h"
#include "xpano/algorithm/image.h"
#include "xpano/constants.h"
#include "xpano/utils/common.h"
#include "xpano/utils/exiv2.h"
#include "xpano/utils/future.h"
#include "xpano/utils/opencv.h"
#include "xpano/utils/threadpool.h"
#include "xpano/utils/vec_opencv.h"

namespace xpano::pipeline {
namespace {

#if XPANO_OPENCV_HAS_JPEG_SUBSAMPLING_SUPPORT
cv::ImwriteJPEGSamplingFactorParams ToOpenCVEnum(
    const ChromaSubsampling &subsampling) {
  switch (subsampling) {
    case ChromaSubsampling::k444:
      return cv::IMWRITE_JPEG_SAMPLING_FACTOR_444;
    case ChromaSubsampling::k422:
      return cv::IMWRITE_JPEG_SAMPLING_FACTOR_422;
    case ChromaSubsampling::k420:
      return cv::IMWRITE_JPEG_SAMPLING_FACTOR_420;
    default:
      return cv::IMWRITE_JPEG_SAMPLING_FACTOR_422;
  }
}
#endif

std::vector<int> CompressionParameters(const CompressionOptions &options) {
  return {
    cv::IMWRITE_JPEG_QUALITY, options.jpeg_quality,
        cv::IMWRITE_JPEG_PROGRESSIVE,
        static_cast<int>(options.jpeg_progressive), cv::IMWRITE_JPEG_OPTIMIZE,
        static_cast<int>(options.jpeg_optimize),
#if XPANO_OPENCV_HAS_JPEG_SUBSAMPLING_SUPPORT
        cv::IMWRITE_JPEG_SAMPLING_FACTOR,
        ToOpenCVEnum(options.jpeg_subsampling),
#endif
        cv::IMWRITE_PNG_COMPRESSION, options.png_compression
  };
}

template <typename TFutureType, RunTraits run>
auto MakeTask() -> std::conditional_t<run == RunTraits::kReturnFuture,
                                      Task<TFutureType>, Task<GenericFuture>> {
  return {.progress = std::make_unique<ProgressMonitor>()};
}

enum class WaitStatus {
  kReady,
  kCancelled,
};

template <typename TFutureType>
WaitStatus WaitWithCancellation(TFutureType *future,
                                ProgressMonitor *progress) {
  std::future_status status;
  while ((status = future->wait_for(kTaskCancellationTimeout)) !=
         std::future_status::ready) {
    if (progress->IsCancelled()) {
      return WaitStatus::kCancelled;
    }
  }
  if (progress->IsCancelled()) {
    return WaitStatus::kCancelled;
  }
  return WaitStatus::kReady;
}

ExportResult RunExportPipeline(cv::Mat pano, const ExportOptions &options,
                               ProgressMonitor *progress) {
  int num_tasks = 2;
  progress->Reset(ProgressType::kExport, num_tasks);

  if (options.crop) {
    auto crop_rect = utils::GetCvRect(pano, *options.crop);
    pano = pano(crop_rect);
  }

  std::optional<std::filesystem::path> export_path;
  if (cv::imwrite(options.export_path.string(), pano,
                  CompressionParameters(options.compression))) {
    export_path = options.export_path;
  }
  progress->NotifyTaskDone();
  if (export_path && utils::exiv2::Enabled()) {
    auto pano_size = utils::ToIntVec(pano.size);
    utils::exiv2::CreateExif(options.metadata_path, *export_path, pano_size);
  }
  progress->NotifyTaskDone();
  return ExportResult{options.pano_id, export_path};
}

std::vector<algorithm::Image> RunLoadingPipeline(
    const std::vector<std::filesystem::path> &inputs,
    const LoadingOptions &options, bool compute_keypoints,
    ProgressMonitor *progress, utils::mt::Threadpool *pool) {
  int num_tasks = static_cast<int>(inputs.size());
  progress->Reset(ProgressType::kDetectingKeypoints, num_tasks);
  utils::mt::MultiFuture<algorithm::Image> loading_future;
  for (const auto &input : inputs) {
    loading_future.push_back(
        pool->submit([options, input, compute_keypoints, progress]() {
          algorithm::Image image(input);
          image.Load({.preview_longer_side = options.preview_longer_side,
                      .compute_keypoints = compute_keypoints});
          progress->NotifyTaskDone();
          return image;
        }));
  }
  if (auto status = WaitWithCancellation(&loading_future, progress);
      status == WaitStatus::kCancelled) {
    return {};
  }
  auto images = loading_future.get();

  auto num_erased =
      std::erase_if(images, [](const auto &img) { return !img.IsLoaded(); });
  if (num_erased > 0) {
    spdlog::warn("Failed to load {} images", num_erased);
  }
  return images;
}

StitcherData RunMatchingPipeline(std::vector<algorithm::Image> images,
                                 const MatchingOptions &options,
                                 ProgressMonitor *progress,
                                 utils::mt::Threadpool *pool) {
  if (images.empty()) {
    return {};
  }

  if (options.type == MatchingType::kNone) {
    return StitcherData{std::move(images)};
  }

  if (options.type == MatchingType::kSinglePano) {
    auto pano = algorithm::SinglePano(static_cast<int>(images.size()));
    return StitcherData{std::move(images), {}, {pano}};
  }

  int num_images = static_cast<int>(images.size());
  int num_neighbors =
      std::min(options.neighborhood_search_size, num_images - 1);
  int num_tasks =
      1 +                                             // FindPanos
      (num_images - num_neighbors) * num_neighbors +  // full n-tuples
      ((num_neighbors - 1) * num_neighbors) / 2;      // non-full (j - i < 0)

  progress->Reset(ProgressType::kMatchingImages, num_tasks);
  utils::mt::MultiFuture<algorithm::Match> matches_future;
  for (int j = 0; j < images.size(); j++) {
    for (int i = std::max(0, j - num_neighbors); i < j; i++) {
      matches_future.push_back(
          pool->submit([i, j, left = images[i], right = images[j],
                        match_conf = options.match_conf, progress]() {
            auto match =
                algorithm::Match{i, j, MatchImages(left, right, match_conf)};
            progress->NotifyTaskDone();
            return match;
          }));
    }
  }
  if (auto status = WaitWithCancellation(&matches_future, progress);
      status == WaitStatus::kCancelled) {
    return {};
  }
  auto matches = matches_future.get();

  auto panos = FindPanos(matches, options.match_threshold);
  progress->NotifyTaskDone();
  return StitcherData{images, matches, panos};
}

StitchingResult RunStitchingPipeline(
    const algorithm::Pano &pano, const std::vector<algorithm::Image> &images,
    const StitchingOptions &options, ProgressMonitor *progress,
    utils::mt::Threadpool *pool, utils::mt::Threadpool *multiblend_pool) {
  int num_images = static_cast<int>(pano.ids.size());
  int num_tasks = algorithm::StitchTasksCount(num_images) + num_images + 1 +
                  static_cast<int>(options.export_path.has_value()) +
                  static_cast<int>(options.full_res);
  progress->Reset(ProgressType::kLoadingImages, num_tasks);
  std::vector<cv::Mat> imgs;
  if (options.full_res) {
    utils::mt::MultiFuture<cv::Mat> imgs_future;
    for (const auto &img_id : pano.ids) {
      imgs_future.push_back(pool->submit([&image = images[img_id], progress]() {
        auto full_res_image = image.GetFullRes();
        progress->NotifyTaskDone();
        return full_res_image;
      }));
    }
    imgs = imgs_future.get();
  } else {
    for (int img_id : pano.ids) {
      imgs.push_back(images[img_id].GetPreview());
      progress->NotifyTaskDone();
    }
  }

  progress->SetTaskType(ProgressType::kStitchingPano);
  auto [status, result, mask] =
      algorithm::Stitch(imgs, options.stitch_algorithm,
                        {.return_pano_mask = options.full_res,
                         .threads_for_multiblend = multiblend_pool,
                         .progress_monitor = progress});
  progress->NotifyTaskDone();

  if (status != algorithm::stitcher::Status::kSuccess) {
    return StitchingResult{
        .pano_id = options.pano_id,
        .full_res = options.full_res,
        .status = status,
    };
  }

  std::optional<utils::RectRRf> auto_crop;
  std::optional<cv::Mat> pano_mask;
  if (options.full_res) {
    pano_mask = mask;
    progress->SetTaskType(ProgressType::kAutoCrop);
    auto_crop = algorithm::FindLargestCrop(mask);
    progress->NotifyTaskDone();
  }

  std::optional<std::filesystem::path> export_path;
  if (options.export_path) {
    std::optional<std::filesystem::path> metadata_path;
    if (options.metadata.copy_from_first_image) {
      const auto &first_image = images[pano.ids[0]];
      metadata_path = first_image.GetPath();
    }

    export_path = RunExportPipeline(result,
                                    {.export_path = *options.export_path,
                                     .metadata_path = metadata_path,
                                     .compression = options.compression},
                                    progress)
                      .export_path;
  }

  return StitchingResult{options.pano_id, options.full_res, status,   result,
                         auto_crop,       export_path,      pano_mask};
}

}  // namespace

using ProgressType = algorithm::ProgressType;

template <RunTraits run>
StitcherPipeline<run>::~StitcherPipeline() {
  Cancel();
}

template <RunTraits run>
void StitcherPipeline<run>::WaitForTasks() {
  pool_.wait_for_tasks();
}

template <RunTraits run>
void StitcherPipeline<run>::Cancel() {
  if (queue_.empty()) {
    return;
  }
  queue_.back().progress->Cancel();
  pool_.purge();
  spdlog::info("Waiting for running tasks to finish");
}

template <RunTraits run>
auto StitcherPipeline<run>::RunLoading(
    const std::vector<std::filesystem::path> &inputs,
    const LoadingOptions &loading_options,
    const MatchingOptions &matching_options)
    -> std::conditional_t<run == RunTraits::kReturnFuture,
                          Task<std::future<StitcherData>>, void> {
  Cancel();
  auto task = MakeTask<std::future<StitcherData>, run>();

  task.future = pool_.submit([this, loading_options, matching_options, inputs,
                              progress = task.progress.get()]() {
    auto images = RunLoadingPipeline(
        inputs, loading_options,
        /*compute_keypoints=*/matching_options.type == MatchingType::kAuto,
        progress, &pool_);
    return RunMatchingPipeline(images, matching_options, progress, &pool_);
  });

  if constexpr (run == RunTraits::kReturnFuture) {
    return task;
  } else {
    queue_.push(std::move(task));
  }
}

template <RunTraits run>
auto StitcherPipeline<run>::RunStitching(const StitcherData &data,
                                         const StitchingOptions &options)
    -> std::conditional_t<run == RunTraits::kReturnFuture,
                          Task<std::future<StitchingResult>>, void> {
  Cancel();
  auto task = MakeTask<std::future<StitchingResult>, run>();

  auto pano = data.panos[options.pano_id];
  task.future = pool_.submit([pano, &images = data.images, options,
                              progress = task.progress.get(), this]() {
    return RunStitchingPipeline(pano, images, options, progress, &pool_,
                                &multiblend_pool_);
  });

  if constexpr (run == RunTraits::kReturnFuture) {
    return task;
  } else {
    queue_.push(std::move(task));
  }
}

template <RunTraits run>
auto StitcherPipeline<run>::RunExport(cv::Mat pano,
                                      const ExportOptions &options)
    -> std::conditional_t<run == RunTraits::kReturnFuture,
                          Task<std::future<ExportResult>>, void> {
  Cancel();
  auto task = MakeTask<std::future<ExportResult>, run>();

  task.future = pool_.submit(
      [pano = std::move(pano), options, progress = task.progress.get()]() {
        return RunExportPipeline(pano, options, progress);
      });

  if constexpr (run == RunTraits::kReturnFuture) {
    return task;
  } else {
    queue_.push(std::move(task));
  }
}

template <RunTraits run>
auto StitcherPipeline<run>::RunInpainting(cv::Mat pano, cv::Mat pano_mask,
                                          const InpaintingOptions &options)
    -> std::conditional_t<run == RunTraits::kReturnFuture,
                          Task<std::future<InpaintingResult>>, void> {
  Cancel();
  auto task = MakeTask<std::future<InpaintingResult>, run>();

  task.future =
      pool_.submit([pano = std::move(pano), pano_mask = std::move(pano_mask),
                    options, progress = task.progress.get()]() {
        int num_tasks = 3;
        progress->Reset(ProgressType::kInpainting, num_tasks);

        cv::Mat inpaint_mask;
        cv::bitwise_not(pano_mask, inpaint_mask);
        progress->NotifyTaskDone();
        int pixels_filled = cv::countNonZero(inpaint_mask);
        progress->NotifyTaskDone();
        auto result = algorithm::Inpaint(pano, inpaint_mask, options);
        progress->NotifyTaskDone();

        return InpaintingResult{result, pixels_filled};
      });

  if constexpr (run == RunTraits::kReturnFuture) {
    return task;
  } else {
    queue_.push(std::move(task));
  }
}

template <RunTraits run>
ProgressReport StitcherPipeline<run>::Progress() const {
  if (queue_.empty()) {
    return {};
  }
  return queue_.back().progress->Report();
}

template <RunTraits run>
auto StitcherPipeline<run>::GetReadyTask()
    -> std::optional<Task<GenericFuture>> {
  if (queue_.empty()) {
    return {};
  }

  if (std::visit(
          [](const auto &future) { return utils::future::IsReady(future); },
          queue_.front().future)) {
    auto task = std::move(queue_.front());
    queue_.pop();
    return task;
  }

  return {};
}

template class StitcherPipeline<RunTraits::kOwnFuture>;
template class StitcherPipeline<RunTraits::kReturnFuture>;

}  // namespace xpano::pipeline
