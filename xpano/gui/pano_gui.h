#pragma once

#include <future>
#include <optional>
#include <string>

#include <opencv2/core.hpp>

#include "algorithm/stitcher_pipeline.h"
#include "gui/action.h"
#include "gui/backends/base.h"
#include "gui/layout.h"
#include "gui/panels/preview_pane.h"
#include "gui/panels/thumbnail_pane.h"
#include "log/logger.h"

namespace xpano::gui {

class PanoGui {
 public:
  explicit PanoGui(backends::Base* backend, logger::LoggerGui* logger);

  bool Run();

 private:
  Action DrawGui();
  Action DrawSidebar();
  Action ResolveFutures();
  void ResetSelections(Action action);
  void PerformAction(Action action);
  void ModifyPano(Action action);

  int selected_image_ = -1;
  int selected_pano_ = -1;
  int selected_match_ = -1;

  std::string info_message_;
  std::string tooltip_message_;

  Layout layout_;
  logger::LoggerGui* logger_;
  PreviewPane plot_pane_;
  ThumbnailPane thumbnail_pane_;
  algorithm::StitcherPipeline stitcher_pipeline_;
  algorithm::CompressionOptions compression_options_;

  std::future<algorithm::StitcherData> stitcher_data_future_;
  std::optional<algorithm::StitcherData> stitcher_data_;

  std::future<algorithm::StitchingResult> pano_future_;
};

}  // namespace xpano::gui
