// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Naachiket Pant
// SPDX-FileCopyrightText: 2022 Vaibhav Sharma
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/pano_gui.h"

#include <algorithm>
#include <future>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <imgui.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "xpano/algorithm/algorithm.h"
#include "xpano/algorithm/image.h"
#include "xpano/cli/args.h"
#include "xpano/constants.h"
#include "xpano/gui/action.h"
#include "xpano/gui/backends/base.h"
#include "xpano/gui/file_dialog.h"
#include "xpano/gui/layout.h"
#include "xpano/gui/panels/preview_pane.h"
#include "xpano/gui/panels/sidebar.h"
#include "xpano/gui/panels/thumbnail_pane.h"
#include "xpano/gui/shortcut.h"
#include "xpano/log/logger.h"
#include "xpano/pipeline/stitcher_pipeline.h"
#include "xpano/utils/common.h"
#include "xpano/utils/config.h"
#include "xpano/utils/imgui_.h"
#include "xpano/version.h"

template <>
struct fmt::formatter<xpano::gui::StatusMessage> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const xpano::gui::StatusMessage& message,
              FormatContext& ctx) const -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "{} {}", message.text, message.tooltip);
  }
};

namespace xpano::gui {

namespace {

void DrawInfoMessage(const StatusMessage& status_message) {
  ImGui::Text("%s", status_message.text.c_str());
  if (!status_message.tooltip.empty()) {
    ImGui::SameLine();
    utils::imgui::InfoMarker("(info)", status_message.tooltip);
  }
}

std::string PreviewMessage(const Selection& selection, ImageType image_type) {
  switch (selection.type) {
    case SelectionType::kImage:
      return fmt::format("Image {}", selection.target_id);
    case SelectionType::kMatch:
      return fmt::format("Match {}", selection.target_id);
    case SelectionType::kPano: {
      if (image_type == ImageType::kPanoFullRes) {
        return fmt::format("Pano {} (Full)", selection.target_id);
      }
      if (image_type == ImageType::kPanoPreview) {
        return fmt::format("Pano {} (Preview)", selection.target_id);
      }
      return fmt::format("Pano {}", selection.target_id);
    }
    default:
      return "";
  }
}

Action ModifyPano(int clicked_image, Selection* selection,
                  std::vector<algorithm::Pano>* panos) {
  // Nothing was selected and an image was ctrl clicked
  if (selection->type == SelectionType::kNone) {
    return {.type = ActionType::kShowImage,
            .target_id = clicked_image,
            .delayed = true};
  }
  // Existing pano is being edited
  if (selection->type == SelectionType::kPano) {
    auto& pano = panos->at(selection->target_id);
    auto iter = std::find(pano.ids.begin(), pano.ids.end(), clicked_image);
    if (iter == pano.ids.end()) {
      pano.ids.push_back(clicked_image);
    } else {
      pano.ids.erase(iter);
    }
    // Pano was deleted
    if (pano.ids.empty()) {
      auto pano_iter = panos->begin() + selection->target_id;
      panos->erase(pano_iter);
      *selection = {};
      return {.type = ActionType::kDisableHighlight, .delayed = true};
    }
  }

  if (selection->type == SelectionType::kImage) {
    // Deselect image
    if (selection->target_id == clicked_image) {
      *selection = {};
      return {.type = ActionType::kDisableHighlight, .delayed = true};
    }

    // Start a new pano from selected image
    panos->push_back({.ids = {selection->target_id, clicked_image}});
    *selection = {SelectionType::kPano, static_cast<int>(panos->size()) - 1};
  }

  // Queue pano stitching
  if (selection->type == SelectionType::kPano) {
    return {.type = ActionType::kShowPano,
            .target_id = selection->target_id,
            .delayed = true};
  }
  return {};
}

auto ResolveStitcherDataFuture(
    std::future<pipeline::StitcherData> stitcher_data_future,
    ThumbnailPane* thumbnail_pane, StatusMessage* status_message)
    -> std::optional<pipeline::StitcherData> {
  std::optional<pipeline::StitcherData> stitcher_data;
  try {
    stitcher_data = stitcher_data_future.get();
  } catch (const std::exception& e) {
    *status_message = {"Couldn't load images", e.what()};
    spdlog::error(*status_message);
    return {};
  }
  if (stitcher_data->images.empty()) {
    *status_message = {"No images loaded"};
    spdlog::info(*status_message);
    return {};
  }

  thumbnail_pane->Load(stitcher_data->images);
  *status_message = {
      fmt::format("Loaded {} images", stitcher_data->images.size())};
  spdlog::info(*status_message);
  return stitcher_data;
}

auto ResolveStitchingResultFuture(
    std::future<pipeline::StitchingResult> pano_future, PreviewPane* plot_pane,
    StatusMessage* status_message) -> pipeline::StitchingResult {
  pipeline::StitchingResult result;
  try {
    result = pano_future.get();
  } catch (const std::exception& e) {
    *status_message = {"Failed to stitch pano", e.what()};
    spdlog::error(*status_message);
    plot_pane->Reset();
    return {};
  }
  if (!result.pano) {
    *status_message = {fmt::format("Failed to stitch pano {}", result.pano_id),
                       algorithm::ToString(result.status)};
    spdlog::info(*status_message);
    if (!result.full_res) {
      plot_pane->Reset();
    }
    return {};
  }

  *status_message = {
      fmt::format("Stitched pano {} successfully", result.pano_id)};
  spdlog::info(*status_message);

  plot_pane->Load(*result.pano, result.full_res ? ImageType::kPanoFullRes
                                                : ImageType::kPanoPreview);

  if (result.auto_crop) {
    plot_pane->SetSuggestedCrop(*result.auto_crop);
  }

  if (result.cameras) {
    plot_pane->SetCameras(*result.cameras);
  }

  if (result.export_path) {
    *status_message = {
        fmt::format("Exported pano {} successfully", result.pano_id),
        result.export_path->string()};
    spdlog::info(*status_message);
  }

  return std::move(result);
}

auto ResolveExportFuture(std::future<pipeline::ExportResult> export_future,
                         PreviewPane* plot_pane, StatusMessage* status_message)
    -> std::optional<int> {
  pipeline::ExportResult result;
  try {
    result = export_future.get();
  } catch (const std::exception& e) {
    *status_message = {"Failed to export pano", e.what()};
    spdlog::error(*status_message);
    return {};
  }

  if (result.export_path) {
    *status_message = {
        fmt::format("Exported pano {} successfully", result.pano_id),
        result.export_path->string()};
    spdlog::info(*status_message);
    plot_pane->EndCrop();
    return result.pano_id;
  }
  return {};
}

auto ResolveInpaintingResultFuture(
    std::future<pipeline::InpaintingResult> inpainting_future,
    PreviewPane* plot_pane, StatusMessage* status_message) -> void {
  pipeline::InpaintingResult result;
  try {
    result = inpainting_future.get();
  } catch (const std::exception& e) {
    *status_message = {"Failed to inpaint pano", e.what()};
    spdlog::error(*status_message);
    return;
  }

  plot_pane->Reload(result.pano, ImageType::kPanoFullRes);

  *status_message = {
      fmt::format("Auto filled {:.1f} MP",
                  static_cast<float>(result.pixels_inpainted) / kMegapixel)};
  spdlog::info(*status_message);
}

bool AnyRawImage(const std::vector<algorithm::Image>& images) {
  return std::any_of(images.begin(), images.end(),
                     [](const auto& img) { return img.IsRaw(); });
}

WarningType GetWarningType(utils::config::LoadingStatus loading_status) {
  switch (loading_status) {
    case utils::config::LoadingStatus::kNoSuchFile:
      return WarningType::kFirstTimeLaunch;
    case utils::config::LoadingStatus::kBreakingChange:
      return WarningType::kUserPrefBreakingChange;
    case utils::config::LoadingStatus::kUnknownError:
      return WarningType::kUserPrefCouldntLoad;
    default:
      return WarningType::kNone;
  }
}

const algorithm::Image* FirstImage(const pipeline::StitcherData& stitcher_data,
                                   int pano_id) {
  const auto& pano = stitcher_data.panos.at(pano_id);
  return &stitcher_data.images.at(pano.ids.at(0));
}

}  // namespace

PanoGui::PanoGui(backends::Base* backend, logger::Logger* logger,
                 const utils::config::Config& config,
                 std::future<utils::Texts> licenses, const cli::Args& args)
    : options_(config.user_options),
      log_pane_(logger),
      about_pane_(std::move(licenses)),
      bugreport_pane_(logger),
      plot_pane_(backend),
      thumbnail_pane_(backend) {
  if (config.app_state.xpano_version != version::Current()) {
    warning_pane_.QueueNewVersion(config.app_state.xpano_version,
                                  about_pane_.GetText(kChangelogFilename));
  }
  if (config.user_options_status != utils::config::LoadingStatus::kSuccess) {
    warning_pane_.Queue(GetWarningType(config.user_options_status));
  }
  if (!args.input_paths.empty()) {
    next_actions_ |=
        Action{.type = ActionType::kLoadFiles, .extra = args.input_paths};
  }
}

bool PanoGui::IsDebugEnabled() const { return log_pane_.IsShown(); }

bool PanoGui::Run() {
  MultiAction actions = std::move(next_actions_);

  actions |= DrawGui();
  actions |= CheckKeybindings();
  actions |= ResolveFutures();

  MultiAction extra_actions;
  for (const auto& action : actions.items) {
    extra_actions |= PerformAction(action);
  }
  actions |= extra_actions;
  next_actions_ = ForwardDelayed(actions);

  return std::any_of(
      actions.items.begin(), actions.items.end(),
      [](const auto& action) { return action.type == ActionType::kQuit; });
}

Action PanoGui::DrawGui() {
  layout::InitDockSpace();
  auto action = DrawSidebar();
  action |= thumbnail_pane_.Draw();
  plot_pane_.Draw(PreviewMessage(selection_, plot_pane_.Type()));
  log_pane_.Draw();
  about_pane_.Draw();
  bugreport_pane_.Draw();
  warning_pane_.Draw();
  return action;
}

Action PanoGui::DrawSidebar() {
  Action action{};
  ImGui::Begin("PanoSweep", nullptr,
               ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar);

  action |= DrawMenu(&options_, IsDebugEnabled());
  DrawWelcomeTextPart1();
  action |= DrawImportActionButtons();
  DrawWelcomeTextPart2();
  action |= DrawActionButtons(plot_pane_.Type(), selection_.target_id,
                              &options_.stitch.projection.type);

  auto progress = stitcher_pipeline_.Progress();
  DrawProgressBar(progress);
  if (progress.tasks_done < progress.num_tasks) {
    if (ImGui::SmallButton("Cancel")) {
      action |= Action{ActionType::kCancelPipeline};
    }
    ImGui::SameLine();
  }
  DrawInfoMessage(status_message_);

  ImGui::Separator();
  ImGui::BeginChild("Panos");
  if (stitcher_data_) {
    auto highlight_id =
        selection_.type == SelectionType::kPano ? selection_.target_id : -1;
    action |=
        DrawPanosMenu(stitcher_data_->panos, thumbnail_pane_, highlight_id);
    if (IsDebugEnabled()) {
      ImGui::SeparatorText("Debug");
      auto highlight_id =
          selection_.type == SelectionType::kMatch ? selection_.target_id : -1;
      action |= DrawMatchesMenu(stitcher_data_->matches, thumbnail_pane_,
                                highlight_id);
    }
  }

  ImGui::EndChild();
  ImGui::End();
  return action;
}

void PanoGui::Reset() {
  thumbnail_pane_.Reset();
  plot_pane_.Reset();
  selection_ = {};
  status_message_ = {};
  pano_mask_ = cv::Mat{};
  // Order of the following lines is important
  stitcher_pipeline_.CancelAndWait();
  stitcher_data_.reset();
}

Action PanoGui::PerformAction(const Action& action) {
  if (action.delayed) {
    return {};
  }

  switch (action.type) {
    default: {
      break;
    }
    case ActionType::kCancelPipeline: {
      stitcher_pipeline_.Cancel();
      break;
    }
    case ActionType::kDisableHighlight: {
      thumbnail_pane_.DisableHighlight();
      break;
    }
    case ActionType::kExport: {
      if (selection_.type == SelectionType::kPano) {
        PerformExportAction(selection_.target_id);
      }
      break;
    }
    case ActionType::kInpaint: {
      if (plot_pane_.Type() == ImageType::kPanoFullRes && pano_mask_) {
        spdlog::info("Auto fill pano {}", selection_.target_id);
        status_message_ = {};
        stitcher_pipeline_.RunInpainting(plot_pane_.Image(), *pano_mask_,
                                         options_.inpaint);
      } else {
        status_message_ = {"Full-resolution panorama not available",
                           "Please rerun full-resolution stitching"};
        spdlog::warn(status_message_);
      }
      break;
    }
    case ActionType::kOpenDirectory:
      [[fallthrough]];
    case ActionType::kOpenFiles: {
      auto files = file_dialog::Open(action);
      if (!files) {
        spdlog::warn(files.error());
        warning_pane_.QueueFilePickerError(files.error());
        break;
      }
      return {.type = ActionType::kLoadFiles, .delayed = true, .extra = *files};
    }
    case ActionType::kLoadFiles: {
      if (auto files = ValueOrDefault<LoadFilesExtra>(action); !files.empty()) {
        Reset();
        stitcher_pipeline_.RunLoading(files, options_.loading,
                                      options_.matching);
      }
      break;
    }
    case ActionType::kShowMatch: {
      selection_ = {SelectionType::kMatch, action.target_id};
      spdlog::info("Clicked match {}", action.target_id);
      const auto& match = stitcher_data_->matches[action.target_id];
      spdlog::info("Match distance {}", match.avg_shift);
      auto img = DrawMatches(match, stitcher_data_->images);
      plot_pane_.Load(img, ImageType::kMatch);
      thumbnail_pane_.SetScrollX(match.id1, match.id2);
      thumbnail_pane_.Highlight(match.id1, match.id2);
      break;
    }
    case ActionType::kShowPano: {
      selection_ = {SelectionType::kPano, action.target_id};
      spdlog::info("Calculating pano {}", selection_.target_id);
      status_message_ = {};
      auto extra = ValueOrDefault<ShowPanoExtra>(action);
      stitcher_pipeline_.RunStitching(*stitcher_data_,
                                      {.pano_id = selection_.target_id,
                                       .full_res = extra.full_res,
                                       .stitch_algorithm = options_.stitch});
      const auto& pano = stitcher_data_->panos[selection_.target_id];
      thumbnail_pane_.Highlight(pano.ids);
      if (extra.scroll_thumbnails) {
        thumbnail_pane_.SetScrollX(pano.ids);
      }
      break;
    }
    case ActionType::kModifyPano: {
      return ModifyPano(action.target_id, &selection_, &stitcher_data_->panos);
    }
    case ActionType::kRotate: {
      if (selection_.type == SelectionType::kPano) {
        if (auto& cameras =
                stitcher_data_->panos.at(selection_.target_id).cameras;
            cameras) {
          auto extra = ValueOrDefault<RotateExtra>(action);
          cameras = algorithm::Rotate(*cameras, extra.rotation_matrix);
        }
      }
    }
      [[fallthrough]];
    case ActionType::kRecomputePano: {
      if (selection_.type == SelectionType::kPano) {
        spdlog::info("Recomputing pano {}: {}", selection_.target_id,
                     Label(options_.stitch.projection.type));
        return {.type = ActionType::kShowPano,
                .target_id = selection_.target_id,
                .delayed = true};
      }
      break;
    }
    case ActionType::kShowImage: {
      selection_ = {SelectionType::kImage, action.target_id};
      const auto& img = stitcher_data_->images[action.target_id];
      plot_pane_.Load(img.Draw(IsDebugEnabled()), ImageType::kSingleImage);
      thumbnail_pane_.Highlight(action.target_id);
      break;
    }
    case ActionType::kShowAbout: {
      about_pane_.Show();
      break;
    }
    case ActionType::kShowBugReport: {
      bugreport_pane_.Show();
      break;
    }
    case ActionType::kToggleDebugLog: {
      log_pane_.ToggleShow();
      break;
    }
    case ActionType::kToggleCrop: {
      plot_pane_.ToggleCrop();
      break;
    }
    case ActionType::kToggleRotate: {
      return plot_pane_.ToggleRotate();
      break;
    }
    case ActionType::kWarnInputConversion: {
      warning_pane_.Queue(WarningType::kWarnInputConversion);
      break;
    }
    case ActionType::kResetOptions: {
      options_ = {};
      warning_pane_.Queue(WarningType::kUserPrefResetOnRequest);
      break;
    }
  }
  return {};
}

void PanoGui::PerformExportAction(int pano_id) {
  spdlog::info("Exporting pano {}", pano_id);
  status_message_ = {};

  const auto* first_image = FirstImage(*stitcher_data_, pano_id);
  auto export_path = file_dialog::Save(first_image->PanoName());

  if (!export_path) {
    spdlog::warn(export_path.error());
    warning_pane_.QueueFilePickerError(export_path.error());
    return;
  }

  if (plot_pane_.Type() == ImageType::kPanoFullRes) {
    std::optional<std::filesystem::path> metadata_path;
    if (options_.metadata.copy_from_first_image) {
      metadata_path = first_image->GetPath();
    }
    stitcher_pipeline_.RunExport(plot_pane_.Image(),
                                 {.pano_id = pano_id,
                                  .export_path = *export_path,
                                  .metadata_path = metadata_path,
                                  .compression = options_.compression,
                                  .crop = plot_pane_.CropRect()});
  } else {
    stitcher_pipeline_.RunStitching(*stitcher_data_,
                                    {.pano_id = pano_id,
                                     .full_res = true,
                                     .export_path = *export_path,
                                     .metadata = options_.metadata,
                                     .compression = options_.compression,
                                     .stitch_algorithm = options_.stitch});
  }
}

MultiAction PanoGui::ResolveFutures() {
  MultiAction actions;
  auto handle_stitcher_data =
      [this,
       &actions](std::future<pipeline::StitcherData> stitcher_data_future) {
        stitcher_data_ =
            ResolveStitcherDataFuture(std::move(stitcher_data_future),
                                      &thumbnail_pane_, &status_message_);

        if (stitcher_data_ && AnyRawImage(stitcher_data_->images)) {
          actions |= {.type = ActionType::kWarnInputConversion};
        }
        if (stitcher_data_ && !stitcher_data_->panos.empty()) {
          // keep delayed == true to wait for the thumbnails to be drawn at
          // leaset once before scrolling
          actions |= {.type = ActionType::kShowPano,
                      .target_id = 0,
                      .delayed = true,
                      .extra = ShowPanoExtra{.scroll_thumbnails = true}};
        }
      };

  auto handle_pano =
      [this](std::future<pipeline::StitchingResult> pano_future) {
        auto result = ResolveStitchingResultFuture(
            std::move(pano_future), &plot_pane_, &status_message_);
        if (result.export_path) {
          stitcher_data_->panos[result.pano_id].exported = true;
        }
        if (result.cameras) {
          stitcher_data_->panos[result.pano_id].cameras = result.cameras;
        }
        pano_mask_ = result.mask;
      };

  auto handle_export =
      [this](std::future<pipeline::ExportResult> export_future) {
        auto exported_pano_id = ResolveExportFuture(
            std::move(export_future), &plot_pane_, &status_message_);
        if (exported_pano_id) {
          stitcher_data_->panos[*exported_pano_id].exported = true;
        }
      };

  auto handle_inpaint =
      [this](std::future<pipeline::InpaintingResult> inpaint_future) {
        ResolveInpaintingResultFuture(std::move(inpaint_future), &plot_pane_,
                                      &status_message_);
      };

  if (auto task = stitcher_pipeline_.GetReadyTask();
      task && task->progress->IsCancelled()) {
    spdlog::info("Task cancelled");
  } else if (task && !task->progress->IsCancelled()) {
    std::visit(utils::Overloaded{handle_stitcher_data, handle_pano,
                                 handle_export, handle_inpaint},
               std::move(task->future));
  }

  return actions;
}

pipeline::Options PanoGui::GetOptions() const { return options_; }

}  // namespace xpano::gui
