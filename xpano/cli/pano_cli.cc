// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/cli/pano_cli.h"

#include <atomic>
#include <filesystem>

#include <spdlog/spdlog.h>

#include "xpano/algorithm/algorithm.h"
#include "xpano/cli/args.h"
#include "xpano/cli/signal.h"
#include "xpano/constants.h"
#include "xpano/log/logger.h"
#include "xpano/pipeline/stitcher_pipeline.h"
#include "xpano/utils/future.h"
#include "xpano/version_fmt.h"

#ifdef _WIN32
#include "xpano/cli/windows_console.h"
#endif

namespace xpano::cli {

namespace {

std::atomic_int cancel = 0;

#ifdef _WIN32
BOOL WINAPI CancelHandler(DWORD event_type) {
  if (event_type == CTRL_C_EVENT) {
    auto previous_cancel_requests = cancel.fetch_add(1);
    if (previous_cancel_requests == 0) {
      return TRUE;  // keep running
    }
  }
  spdlog::info("Shutdown, press ENTER to continue.");
  return FALSE;  // exit
}
#else
void CancelHandler(int /*signal*/) { cancel.fetch_add(1); }
#endif

void PrintVersion() { spdlog::info("Xpano version {}", version::Current()); }

ResultType RunPipeline(const Args &args) {
  pipeline::StitcherPipeline<pipeline::RunTraits::kReturnFuture> pipeline;

  auto loading_task = pipeline.RunLoading(
      args.input_paths, {.preview_longer_side = kMaxImageSizeForCLI},
      {.type = pipeline::MatchingType::kSinglePano});

  pipeline::StitcherData stitcher_data;

  try {
    stitcher_data = utils::future::GetWithCancellation(
        std::move(loading_task.future), cancel);
  } catch (const utils::future::Cancelled) {
    spdlog::info("Canceling, press CTRL+C again to force quit.");
    loading_task.progress->Cancel();
    pipeline.WaitForTasks();
    return ResultType::kError;
  } catch (const std::exception &e) {
    spdlog::error("Failed to load images: {}", e.what());
    return ResultType::kError;
  }

  if (stitcher_data.images.empty()) {
    spdlog::error("Failed to load any images");
    return ResultType::kError;
  }

  auto export_path =
      args.output_path
          ? *args.output_path
          : std::filesystem::path(stitcher_data.images[0].PanoName());

  auto stitching_task = pipeline.RunStitching(
      stitcher_data, {.pano_id = 0, .export_path = export_path});

  pipeline::StitchingResult stitching_result;

  try {
    stitching_result = utils::future::GetWithCancellation(
        std::move(stitching_task.future), cancel);
  } catch (const utils::future::Cancelled) {
    spdlog::info("Canceling, press CTRL+C again to force quit.");
    stitching_task.progress->Cancel();
    pipeline.WaitForTasks();
    return ResultType::kError;
  } catch (const std::exception &e) {
    spdlog::error("Failed to stitch panorama: {}", e.what());
    return ResultType::kError;
  }

  if (!stitching_result.pano) {
    spdlog::error("Failed to stitch panorama: {}",
                  algorithm::ToString(stitching_result.status));
    return ResultType::kError;
  }

  if (!stitching_result.export_path) {
    spdlog::error("Failed to export panorama to file: {}",
                  export_path.string());
    return ResultType::kError;
  }

  spdlog::info("Successfully exported to {}",
               stitching_result.export_path->string());
  spdlog::info("Size: {} x {}", stitching_result.pano->cols,
               stitching_result.pano->rows);

  return ResultType::kSuccess;
}
}  // namespace

std::pair<ResultType, std::optional<Args>> Run(int argc, char **argv) {
#ifdef _WIN32
  auto attach_console = windows::Attach();
#endif
  logger::RedirectSpdlogToCout();

  auto args = ParseArgs(argc, argv);

  if (!args) {
    PrintHelp();
    return {ResultType::kError, std::nullopt};
  }

  if (args->print_help) {
    PrintHelp();
    return {ResultType::kSuccess, std::nullopt};
  }

  if (args->print_version) {
    PrintVersion();
    return {ResultType::kSuccess, std::nullopt};
  }

  if (args->run_gui || args->input_paths.empty()) {
    return {ResultType::kForwardToGui, args};
  }

  signal::RegisterInterruptHandler(CancelHandler);
  return {RunPipeline(*args), args};
}

int ExitCode(ResultType result) {
  if (result == ResultType::kSuccess) {
    return 0;
  }
  return -1;
}

}  // namespace xpano::cli
