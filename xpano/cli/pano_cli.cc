#include "xpano/cli/pano_cli.h"

#include <spdlog/spdlog.h>

#include "xpano/cli/args.h"
#include "xpano/pipeline/stitcher_pipeline.h"
#include "xpano/utils/path.h"

namespace xpano::cli {

inline void RunXPanoCLI(const Args &args) {
  pipeline::StitcherPipeline pipeline;

  // auto images =
  //     pipeline.RunLoadingPipeline(utils::path::ToString(args.input_paths),
  //     {});

  // if (images.empty()) {
  //   spdlog::error("Failed to load any images");
  //   return;
  // }

  // auto result = pipeline.RunStitchingPipeline(
  //     {}, images, {.pano_id = 0, .full_res = true, .export_path = {}});

  // if (result.export_path) {
  //   spdlog::info("Exported to {}", *result.export_path);
  //   spdlog::info(" - size = {}x{}", result.pano->cols, result.pano->rows);
  // }
}

}  // namespace xpano::cli
