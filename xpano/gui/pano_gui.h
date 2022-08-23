#pragma once

#include <future>
#include <optional>
#include <string>

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

template <typename TType>
bool IsReady(const std::future<TType>& future);

struct StatusMessage {
  std::string text;
  std::string tooltip;
};

enum class SelectionType {
  kNone,
  kImage,
  kMatch,
  kPano,
};

struct Selection {
  SelectionType type = SelectionType::kNone;
  int target_id = -1;
};

class PanoGui {
 public:
  PanoGui(backends::Base* backend, logger::LoggerGui* logger,
          std::future<utils::Texts> licenses);

  bool Run();

 private:
  Action DrawGui();
  Action DrawSidebar();
  Action ResolveFutures();
  Action PerformAction(Action action);
  void Reset();

  Selection selection_;
  Action delayed_action_ = {ActionType::kNone};
  StatusMessage status_message_;

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
