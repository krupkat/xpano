#pragma once

#include <atomic>
#include <future>
#include <optional>
#include <string>
#include <vector>

#include <BS_thread_pool.hpp>
#include <opencv2/core.hpp>
#include <opencv2/stitching.hpp>

#include "xpano/algorithm/algorithm.h"
#include "xpano/algorithm/image.h"
#include "xpano/constants.h"
#include "xpano/utils/rect.h"

namespace xpano::pipeline {

using InpaintingOptions = algorithm::InpaintingOptions;
using ProjectionOptions = algorithm::ProjectionOptions;

struct CompressionOptions {
  int jpeg_quality = kDefaultJpegQuality;
  bool jpeg_progressive = false;
  bool jpeg_optimize = false;
  int png_compression = kDefaultPngCompression;
};

struct MatchingOptions {
  int neighborhood_search_size = kDefaultNeighborhoodSearchSize;
  int match_threshold = kDefaultMatchThreshold;
};

struct LoadingOptions {
  int preview_longer_side = kDefaultPreviewLongerSide;
};

struct StitchingOptions {
  int pano_id = 0;
  bool full_res = false;
  std::optional<std::string> export_path;
  CompressionOptions compression;
  ProjectionOptions projection;
};

struct ExportOptions {
  int pano_id = 0;
  std::string export_path;
  CompressionOptions compression;
  utils::RectRRf crop;
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
  std::optional<std::string> export_path;
  std::optional<cv::Mat> mask;
};

struct ExportResult {
  int pano_id = 0;
  std::optional<std::string> export_path;
};

enum class ProgressType {
  kNone,
  kLoadingImages,
  kStitchingPano,
  kAutoCrop,
  kDetectingKeypoints,
  kMatchingImages,
  kExport,
  kInpainting,
};

struct ProgressReport {
  ProgressType type;
  int tasks_done;
  int num_tasks;
};

class ProgressMonitor {
 public:
  void Reset(ProgressType type, int num_tasks);
  void SetNumTasks(int num_tasks);
  void SetTaskType(ProgressType type);
  [[nodiscard]] ProgressReport Progress() const;
  void NotifyTaskDone();

 private:
  std::atomic<ProgressType> type_{ProgressType::kNone};
  std::atomic<int> done_ = 0;
  std::atomic<int> num_tasks_ = 0;
};

class StitcherPipeline {
 public:
  StitcherPipeline() = default;
  ~StitcherPipeline();
  std::future<StitcherData> RunLoading(const std::vector<std::string> &inputs,
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
      const std::vector<std::string> &inputs, const LoadingOptions &options);
  StitcherData RunMatchingPipeline(std::vector<algorithm::Image> images,
                                   const MatchingOptions &options);
  StitchingResult RunStitchingPipeline(
      const algorithm::Pano &pano, const std::vector<algorithm::Image> &images,
      const StitchingOptions &options);

  ProgressMonitor progress_;

  std::atomic<bool> cancel_tasks_ = false;
  BS::thread_pool pool_;
};

}  // namespace xpano::pipeline
