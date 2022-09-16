#include "xpano/gui/pano_gui.h"

#include <algorithm>
#include <future>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "xpano/algorithm/algorithm.h"
#include "xpano/algorithm/image.h"
#include "xpano/algorithm/stitcher_pipeline.h"
#include "xpano/gui/action.h"
#include "xpano/gui/backends/base.h"
#include "xpano/gui/file_dialog.h"
#include "xpano/gui/layout.h"
#include "xpano/gui/panels/preview_pane.h"
#include "xpano/gui/panels/sidebar.h"
#include "xpano/gui/panels/thumbnail_pane.h"
#include "xpano/gui/shortcut.h"
#include "xpano/log/logger.h"
#include "xpano/utils/future.h"
#include "xpano/utils/imgui_.h"

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
}  // namespace

PanoGui::PanoGui(backends::Base* backend, logger::Logger* logger,
                 std::future<utils::Texts> licenses)
    : plot_pane_(backend),
      thumbnail_pane_(backend),
      log_pane_(logger),
      about_pane_(std::move(licenses)) {}

bool PanoGui::Run() {
  Action action{};
  std::swap(action, delayed_action_);

  action |= DrawGui();
  action |= CheckKeybindings();
  action |= ResolveFutures();
  if (action.type != ActionType::kNone) {
    action |= PerformAction(action);
  }

  if (action.delayed) {
    delayed_action_ = RemoveDelay(action);
  }
  return action.type == ActionType::kQuit;
}

Action PanoGui::DrawGui() {
  layout::InitDockSpace();
  auto action = DrawSidebar();
  action |= thumbnail_pane_.Draw();
  plot_pane_.Draw(PreviewMessage(selection_, plot_pane_.Type()));
  log_pane_.Draw();
  about_pane_.Draw();
  return action;
}

Action PanoGui::DrawSidebar() {
  Action action{};
  ImGui::Begin("PanoSweep", nullptr,
               ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar);
  action |= DrawMenu(&compression_options_, &loading_options_,
                     &matching_options_, &projection_options_);

  DrawWelcomeText();
  action |= DrawActionButtons(plot_pane_.Type(), selection_.target_id);
  ImGui::Spacing();

  auto progress = stitcher_pipeline_.LoadingProgress();
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
    if (log_pane_.IsShown()) {
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
  // Order of the following two lines is important
  stitcher_pipeline_.Cancel();
  stitcher_data_.reset();
}

Action PanoGui::PerformAction(Action action) {
  if (action.delayed) {
    return action;
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
        spdlog::info("Exporting pano {}", selection_.target_id);
        status_message_ = {};
        auto default_name = fmt::format("pano_{}.jpg", selection_.target_id);
        if (auto export_path = file_dialog::Save(default_name); export_path) {
          if (plot_pane_.Type() == ImageType::kPanoFullRes) {
            export_future_ = stitcher_pipeline_.RunExport(
                plot_pane_.Image(), {.pano_id = selection_.target_id,
                                     .export_path = *export_path,
                                     .compression = compression_options_,
                                     .crop_start = plot_pane_.CropStart(),
                                     .crop_end = plot_pane_.CropEnd()});
          } else {
            pano_future_ = stitcher_pipeline_.RunStitching(
                *stitcher_data_, {.pano_id = selection_.target_id,
                                  .full_res = true,
                                  .export_path = *export_path,
                                  .compression = compression_options_,
                                  .projection = projection_options_});
          }
        }
      }
      break;
    }
    case ActionType::kOpenDirectory:
      [[fallthrough]];
    case ActionType::kOpenFiles: {
      if (auto results = file_dialog::Open(action); !results.empty()) {
        Reset();
        stitcher_data_future_ = stitcher_pipeline_.RunLoading(
            results, loading_options_, matching_options_);
      }
      break;
    }
    case ActionType::kShowMatch: {
      selection_ = {SelectionType::kMatch, action.target_id};
      spdlog::info("Clicked match {}", action.target_id);
      const auto& match = stitcher_data_->matches[action.target_id];
      auto img = DrawMatches(match, stitcher_data_->images);
      plot_pane_.Load(img, ImageType::kMatch);
      thumbnail_pane_.SetScrollX(match.id1, match.id2);
      thumbnail_pane_.Highlight(match.id1, match.id2);
      break;
    }
    case ActionType::kShowFullResPano:
      [[fallthrough]];
    case ActionType::kShowPano: {
      selection_ = {SelectionType::kPano, action.target_id};
      spdlog::info("Calculating pano preview {}", selection_.target_id);
      status_message_ = {};
      pano_future_ = stitcher_pipeline_.RunStitching(
          *stitcher_data_,
          {.pano_id = selection_.target_id,
           .full_res = action.type == ActionType::kShowFullResPano,
           .projection = projection_options_});
      const auto& pano = stitcher_data_->panos[selection_.target_id];
      thumbnail_pane_.SetScrollX(pano.ids);
      thumbnail_pane_.Highlight(pano.ids);
      break;
    }
    case ActionType::kModifyPano: {
      return ModifyPano(action.target_id, &selection_, &stitcher_data_->panos);
    }
    case ActionType::kRecomputePano: {
      if (selection_.type == SelectionType::kPano) {
        spdlog::info("Recomputing pano {}: {}", selection_.target_id,
                     Label(projection_options_.projection_type));
        return {.type = ActionType::kShowPano,
                .target_id = selection_.target_id,
                .delayed = true};
      }
      break;
    }
    case ActionType::kShowImage: {
      selection_ = {SelectionType::kImage, action.target_id};
      const auto& img = stitcher_data_->images[action.target_id];
      plot_pane_.Load(img.Draw(log_pane_.IsShown()), ImageType::kSingleImage);
      thumbnail_pane_.Highlight(action.target_id);
      break;
    }
    case ActionType::kShowAbout: {
      about_pane_.Show();
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
  }
  return action;
}

Action PanoGui::ResolveFutures() {
  if (utils::future::IsReady(stitcher_data_future_)) {
    try {
      stitcher_data_ = stitcher_data_future_.get();
    } catch (const std::exception& e) {
      status_message_ = {"Couldn't load images", e.what()};
      spdlog::error(status_message_);
      return {};
    }
    if (stitcher_data_->images.empty()) {
      status_message_ = {"No images loaded"};
      spdlog::info(status_message_);
      stitcher_data_.reset();
      return {};
    }

    thumbnail_pane_.Load(stitcher_data_->images);
    status_message_ = {
        fmt::format("Loaded {} images", stitcher_data_->images.size())};
    spdlog::info(status_message_);
    if (!stitcher_data_->panos.empty()) {
      return {.type = ActionType::kShowPano, .target_id = 0, .delayed = true};
    }
  }

  if (utils::future::IsReady(pano_future_)) {
    algorithm::StitchingResult result;
    try {
      result = pano_future_.get();
      if (result.pano) {
        plot_pane_.Load(*result.pano, result.full_res
                                          ? ImageType::kPanoFullRes
                                          : ImageType::kPanoPreview);
      }
      if (result.auto_crop) {
        plot_pane_.SetSuggestedCrop(*result.auto_crop);
      }
    } catch (const std::exception& e) {
      status_message_ = {"Failed to stitch pano", e.what()};
      spdlog::error(status_message_);
      plot_pane_.Reset();
      return {};
    }
    if (!result.pano) {
      status_message_ = {
          fmt::format("Failed to stitch pano {}", result.pano_id),
          algorithm::ToString(result.status)};
      spdlog::info(status_message_);
      plot_pane_.Reset();
      return {};
    }

    status_message_ = {
        fmt::format("Stitched pano {} successfully", result.pano_id)};
    spdlog::info(status_message_);
    if (result.export_path) {
      status_message_ = {
          fmt::format("Exported pano {} successfully", result.pano_id),
          *result.export_path};
      spdlog::info(status_message_);
      stitcher_data_->panos[result.pano_id].exported = true;
    }
  }

  if (utils::future::IsReady(export_future_)) {
    algorithm::ExportResult result;
    try {
      result = export_future_.get();
    } catch (const std::exception& e) {
      status_message_ = {"Failed to export pano", e.what()};
      spdlog::error(status_message_);
      return {};
    }

    if (result.export_path) {
      status_message_ = {
          fmt::format("Exported pano {} successfully", result.pano_id),
          *result.export_path};
      spdlog::info(status_message_);
      stitcher_data_->panos[result.pano_id].exported = true;
      plot_pane_.EndCrop();
    }
  }
  return {};
}

}  // namespace xpano::gui
