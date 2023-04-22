#include "xpano/pipeline/stitcher_pipeline.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <future>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <BS_thread_pool.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/stitching.hpp>
#include <spdlog/spdlog.h>

#include "xpano/algorithm/algorithm.h"
#include "xpano/algorithm/image.h"
#include "xpano/constants.h"
#include "xpano/utils/opencv.h"
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

    progress_.Reset(ProgressType::kNone, 0);
  }
}

std::future<StitcherData> StitcherPipeline::RunLoading(
    const std::vector<std::filesystem::path> &inputs,
    const LoadingOptions &loading_options,
    const MatchingOptions &matching_options) {
  return pool_.submit([this, loading_options, matching_options, inputs]() {
    auto images = RunLoadingPipeline(
        inputs, loading_options,
        /*compute_keypoints=*/matching_options.type == MatchingType::kAuto);
    return RunMatchingPipeline(images, matching_options);
  });
}

std::future<StitchingResult> StitcherPipeline::RunStitching(
    const StitcherData &data, const StitchingOptions &options) {
  auto pano = data.panos[options.pano_id];
  return pool_.submit([pano, &images = data.images, options, this]() {
    return RunStitchingPipeline(pano, images, options);
  });
}

StitchingResult StitcherPipeline::RunStitchingPipeline(
    const algorithm::Pano &pano, const std::vector<algorithm::Image> &images,
    const StitchingOptions &options) {
  int num_tasks = static_cast<int>(pano.ids.size()) + 1 +
                  static_cast<int>(options.export_path.has_value()) +
                  static_cast<int>(options.full_res);
  progress_.Reset(ProgressType::kLoadingImages, num_tasks);
  std::vector<cv::Mat> imgs;
  if (options.full_res) {
    BS::multi_future<cv::Mat> imgs_future;
    for (const auto &img_id : pano.ids) {
      imgs_future.push_back(pool_.submit([this, &image = images[img_id]]() {
        auto full_res_image = image.GetFullRes();
        progress_.NotifyTaskDone();
        return full_res_image;
      }));
    }
    imgs = imgs_future.get();
  } else {
    for (int img_id : pano.ids) {
      imgs.push_back(images[img_id].GetPreview());
      progress_.NotifyTaskDone();
    }
  }

  progress_.SetTaskType(ProgressType::kStitchingPano);
  auto [status, result, mask] =
      algorithm::Stitch(imgs, options.stitch_algorithm,
                        /*return_pano_mask=*/options.full_res);
  progress_.NotifyTaskDone();

  if (status != cv::Stitcher::OK) {
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
    progress_.SetTaskType(ProgressType::kAutoCrop);
    auto_crop = algorithm::FindLargestCrop(mask);
    progress_.NotifyTaskDone();
  }

  std::optional<std::filesystem::path> export_path;
  if (options.export_path) {
    progress_.SetTaskType(ProgressType::kExport);
    if (cv::imwrite(options.export_path->string(), result,
                    CompressionParameters(options.compression))) {
      export_path = options.export_path;
    }
    progress_.NotifyTaskDone();
  }

  return StitchingResult{options.pano_id, options.full_res, status,   result,
                         auto_crop,       export_path,      pano_mask};
}

std::future<InpaintingResult> StitcherPipeline::RunInpainting(
    cv::Mat pano, cv::Mat pano_mask, const InpaintingOptions &options) {
  return pool_.submit([pano = std::move(pano), pano_mask = std::move(pano_mask),
                       options, this]() {
    int num_tasks = 3;
    progress_.Reset(ProgressType::kInpainting, num_tasks);

    cv::Mat inpaint_mask;
    cv::bitwise_not(pano_mask, inpaint_mask);
    progress_.NotifyTaskDone();
    int pixels_filled = cv::countNonZero(inpaint_mask);
    progress_.NotifyTaskDone();
    auto result = algorithm::Inpaint(pano, inpaint_mask, options);
    progress_.NotifyTaskDone();

    return InpaintingResult{result, pixels_filled};
  });
}

std::future<ExportResult> StitcherPipeline::RunExport(
    cv::Mat pano, const ExportOptions &options) {
  return pool_.submit([pano = std::move(pano), options, this]() {
    int num_tasks = 1;
    progress_.Reset(ProgressType::kExport, num_tasks);

    auto crop_rect = utils::GetCvRect(pano, options.crop);

    std::optional<std::filesystem::path> export_path;
    if (cv::imwrite(options.export_path.string(), pano(crop_rect),
                    CompressionParameters(options.compression))) {
      export_path = options.export_path;
    }
    progress_.NotifyTaskDone();
    return ExportResult{options.pano_id, export_path};
  });
}

ProgressReport StitcherPipeline::Progress() const {
  return progress_.Progress();
}

std::vector<algorithm::Image> StitcherPipeline::RunLoadingPipeline(
    const std::vector<std::filesystem::path> &inputs,
    const LoadingOptions &options, bool compute_keypoints) {
  int num_tasks = static_cast<int>(inputs.size());
  progress_.Reset(ProgressType::kDetectingKeypoints, num_tasks);
  BS::multi_future<algorithm::Image> loading_future;
  for (const auto &input : inputs) {
    loading_future.push_back(
        pool_.submit([this, options, input, compute_keypoints]() {
          algorithm::Image image(input);
          image.Load({.preview_longer_side = options.preview_longer_side,
                      .compute_keypoints = compute_keypoints});
          progress_.NotifyTaskDone();
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
      std::erase_if(images, [](const auto &img) { return !img.IsLoaded(); });
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

  progress_.Reset(ProgressType::kMatchingImages, num_tasks);
  BS::multi_future<algorithm::Match> matches_future;
  for (int j = 0; j < images.size(); j++) {
    for (int i = std::max(0, j - num_neighbors); i < j; i++) {
      matches_future.push_back(
          pool_.submit([this, i, j, left = images[i], right = images[j],
                        match_conf = options.match_conf]() {
            auto match =
                algorithm::Match{i, j, MatchImages(left, right, match_conf)};
            progress_.NotifyTaskDone();
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
  progress_.NotifyTaskDone();
  return StitcherData{images, matches, panos};
}

}  // namespace xpano::pipeline
