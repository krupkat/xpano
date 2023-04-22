#include "xpano/cli/pano_cli.h"

#include <filesystem>

#include <spdlog/spdlog.h>

#include "xpano/algorithm/algorithm.h"
#include "xpano/cli/args.h"
#include "xpano/constants.h"
#include "xpano/pipeline/stitcher_pipeline.h"
#include "xpano/utils/path.h"

namespace xpano::cli {

namespace {}

int Run(const Args &args) {
  pipeline::StitcherPipeline pipeline;

  auto stitcher_data_future = pipeline.RunLoading(
      args.input_paths, {.preview_longer_side = kMaxImageSizeForCLI},
      {.type = pipeline::MatchingType::kSinglePano});

  pipeline::StitcherData stitcher_data;

  try {
    stitcher_data = stitcher_data_future.get();
  } catch (const std::exception &e) {
    spdlog::error("Failed to load images: {}", e.what());
    return -1;
  }

  if (stitcher_data.images.empty()) {
    spdlog::error("Failed to load any images");
    return -1;
  }

  auto export_path =
      args.output_path
          ? *args.output_path
          : std::filesystem::path(stitcher_data.images[0].PanoName());

  auto stitching_result_future = pipeline.RunStitching(
      stitcher_data, {.pano_id = 0, .export_path = export_path});

  pipeline::StitchingResult stitching_result;

  try {
    stitching_result = stitching_result_future.get();
  } catch (const std::exception &e) {
    spdlog::error("Failed to stitch panorama: {}", e.what());
    return -1;
  }

  if (!stitching_result.pano) {
    spdlog::error("Failed to stitch panorama: {}",
                  algorithm::ToString(stitching_result.status));
    return -1;
  }

  if (!stitching_result.export_path) {
    spdlog::error("Failed to export panorama to file: {}",
                  export_path.string());
    return -1;
  }

  spdlog::info("Successfully exported to {}",
               stitching_result.export_path->string());
  spdlog::info("Size: {} x {}", stitching_result.pano->cols,
               stitching_result.pano->rows);

  return 0;
}

}  // namespace xpano::cli
