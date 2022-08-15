#pragma once

#include <future>
#include <optional>
#include <string>
#include <vector>

#include "algorithm/stitcher_pipeline.h"
#include "gui/action.h"
#include "gui/backends/base.h"
#include "gui/layout.h"
#include "gui/panels/about.h"
#include "gui/panels/preview_pane.h"
#include "gui/panels/thumbnail_pane.h"
#include "log/logger.h"
#include "utils/text.h"

namespace xpano::gui {

class PanoGui {
 public:
  PanoGui(backends::Base* backend, logger::LoggerGui* logger,
          std::vector<utils::Text> licenses);

  bool Run();

 private:
  Action DrawGui();
  Action DrawSidebar();
  Action ResolveFutures();
  void ResetSelections(Action action);
  Action PerformAction(Action action);
  Action ModifyPano(Action action);
  std::string PreviewMessage() const;

  int selected_image_ = -1;
  int selected_pano_ = -1;
  int selected_match_ = -1;
  Action delayed_action_ = {ActionType::kNone};

  std::string info_message_;
  std::string tooltip_message_;

  Layout layout_;
  logger::LoggerGui* logger_;
  AboutPane about_pane_;
  PreviewPane plot_pane_;
  ThumbnailPane thumbnail_pane_;
  algorithm::StitcherPipeline stitcher_pipeline_;
  algorithm::CompressionOptions compression_options_;

  std::future<algorithm::StitcherData> stitcher_data_future_;
  std::optional<algorithm::StitcherData> stitcher_data_;

  std::future<algorithm::StitchingResult> pano_future_;
};

}  // namespace xpano::gui
