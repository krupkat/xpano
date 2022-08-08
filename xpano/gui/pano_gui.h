#pragma once

#include <future>
#include <optional>

#include <opencv2/core.hpp>

#include "algorithm/stitcher_pipeline.h"
#include "gui/action.h"
#include "gui/backends/base.h"
#include "gui/layout.h"
#include "gui/preview_pane.h"
#include "gui/thumbnail_pane.h"
#include "log/logger.h"

namespace xpano::gui {

class PanoGui {
 public:
  explicit PanoGui(backends::Base* backend, logger::LoggerGui* logger);

  void Run();

 private:
  Action DrawGui();
  Action DrawSidebar();
  void ResetSelections(Action action);
  void PerformAction(Action action);
  void ResolveFutures();

  int selected_pano_ = -1;
  int selected_match_ = -1;

  Layout layout_;
  logger::LoggerGui* logger_;
  PreviewPane plot_pane_;
  ThumbnailPane thumbnail_pane_;
  algorithm::StitcherPipeline stitcher_pipeline_;

  std::future<algorithm::StitcherData> stitcher_data_future_;
  std::optional<algorithm::StitcherData> stitcher_data_;

  std::future<std::optional<cv::Mat>> pano_future_;
};

}  // namespace xpano::gui
