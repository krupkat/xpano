#pragma once

#include <future>
#include <optional>
#include <string>

#include <opencv2/core.hpp>

#include "xpano/gui/action.h"
#include "xpano/gui/backends/base.h"
#include "xpano/gui/panels/about.h"
#include "xpano/gui/panels/bugreport_pane.h"
#include "xpano/gui/panels/log_pane.h"
#include "xpano/gui/panels/preview_pane.h"
#include "xpano/gui/panels/thumbnail_pane.h"
#include "xpano/gui/panels/warning_pane.h"
#include "xpano/log/logger.h"
#include "xpano/pipeline/options.h"
#include "xpano/pipeline/stitcher_pipeline.h"
#include "xpano/utils/config.h"
#include "xpano/utils/text.h"

namespace xpano::gui {

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
  PanoGui(backends::Base* backend, logger::Logger* logger,
          const utils::config::Config& config,
          std::future<utils::Texts> licenses);

  bool Run();
  pipeline::Options GetOptions() const;

 private:
  Action DrawGui();
  Action DrawSidebar();
  MultiAction ResolveFutures();
  Action PerformAction(Action action);
  void Reset();
  bool IsDebugEnabled() const;

  // PODs
  Selection selection_;
  MultiAction delayed_actions_;
  StatusMessage status_message_;

  pipeline::Options options_;
  std::optional<pipeline::StitcherData> stitcher_data_;

  // Gui panels
  LogPane log_pane_;
  AboutPane about_pane_;
  BugReportPane bugreport_pane_;
  PreviewPane plot_pane_;
  ThumbnailPane thumbnail_pane_;
  WarningPane warning_pane_;

  // Algorithm
  pipeline::StitcherPipeline stitcher_pipeline_;
  std::future<pipeline::StitcherData> stitcher_data_future_;
  std::future<pipeline::StitchingResult> pano_future_;
  std::future<pipeline::ExportResult> export_future_;
  std::future<pipeline::InpaintingResult> inpaint_future_;

  // Used for inpainting
  std::optional<cv::Mat> pano_mask_;
};

}  // namespace xpano::gui
