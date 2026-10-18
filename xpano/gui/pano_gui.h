// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Naachiket Pant
// SPDX-FileCopyrightText: 2022 Vaibhav Sharma
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <future>
#include <optional>
#include <string>

#include <opencv2/core.hpp>

#include "xpano/cli/args.h"
#include "xpano/gui/action.h"
#include "xpano/gui/backends/base.h"
#include "xpano/gui/file_dialog/base.h"
#include "xpano/gui/file_dialog/error.h"
#include "xpano/gui/panels/about.h"
#include "xpano/gui/panels/bugreport_pane.h"
#include "xpano/gui/panels/log_pane.h"
#include "xpano/gui/panels/preview_pane.h"
#include "xpano/gui/panels/sidebar.h"
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

enum class SelectionType : std::uint8_t {
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
  PanoGui(backends::Base* backend, file_dialog::Base* file_dialog_backend,
          logger::Logger* logger, const utils::config::Config& config,
          std::future<utils::Texts> licenses, const cli::Args& args);

  bool Run();
  pipeline::Options GetOptions() const;
  void SetDpiScale(float scale);
  float GetScale() const;

 private:
  Action DrawGui();
  Action DrawSidebar();
  MultiAction ResolveStitcherFutures();
  Action ResolveFileDialogFutures();
  Action PerformAction(const Action& action);

  void RunExportAction(int pano_id, const std::filesystem::path& export_path);

  bool FileDialogPending() const;
  void Reset();
  void SetUiScalePercent(int percent);
  void UpdateUiScale();
  bool IsDebugEnabled() const;

  // File dialogs
  file_dialog::Base* file_dialog_backend_;
  file_dialog::Future file_dialog_future_;

  // PODs
  Selection selection_;
  MultiAction next_actions_;
  StatusMessage status_message_;

  pipeline::Options options_;
  UiScale ui_scale_;
  std::optional<pipeline::StitcherData> stitcher_data_;

  // Gui panels
  LogPane log_pane_;
  AboutPane about_pane_;
  BugReportPane bugreport_pane_;
  PreviewPane plot_pane_;
  ThumbnailPane thumbnail_pane_;
  WarningPane warning_pane_;

  // Algorithm
  pipeline::StitcherPipeline<> stitcher_pipeline_;

  // Used for inpainting
  std::optional<cv::Mat> pano_mask_;
};

}  // namespace xpano::gui
