#pragma once

#include <future>
#include <optional>

#include <opencv2/core.hpp>
#include <SDL.h>

#include "action.h"
#include "layout.h"
#include "logger.h"
#include "preview_pane.h"
#include "stitcher_pipeline.h"
#include "thumbnail_pane.h"

namespace xpano::gui {

class PanoGui {
 public:
  explicit PanoGui(SDL_Renderer* renderer, logger::LoggerGui* logger);

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
  StitcherPipeline stitcher_pipeline_;

  std::future<StitcherData> stitcher_data_future_;
  std::optional<StitcherData> stitcher_data_;

  std::future<std::optional<cv::Mat>> pano_future_;
};

}  // namespace xpano::gui
