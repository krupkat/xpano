#include "gui/pano_gui.h"

#include <algorithm>
#include <future>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include "algorithm/algorithm.h"
#include "algorithm/image.h"
#include "algorithm/stitcher_pipeline.h"
#include "gui/action.h"
#include "gui/backends/base.h"
#include "gui/file_dialog.h"
#include "gui/layout.h"
#include "gui/panels/preview_pane.h"
#include "gui/panels/sidebar.h"
#include "gui/panels/thumbnail_pane.h"
#include "log/logger.h"
#include "utils/future.h"
#include "utils/imgui_.h"

namespace xpano::gui {

namespace {

Action CheckKeybindings() {
  bool ctrl = ImGui::GetIO().KeyCtrl;
  if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O)) {
    return {ActionType::kOpenFiles};
  }
  if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
    return {ActionType::kExport};
  }
  if (ctrl && ImGui::IsKeyPressed(ImGuiKey_D)) {
    return {ActionType::kToggleDebugLog};
  }
  return {ActionType::kNone};
}

void DrawInfoMessage(const StatusMessage& status_message) {
  ImGui::Text("%s", status_message.text.c_str());
  if (!status_message.tooltip.empty()) {
    ImGui::SameLine();
    utils::imgui::InfoMarker("(info)", status_message.tooltip);
  }
}

std::string PreviewMessage(const Selection& selection) {
  switch (selection.type) {
    case SelectionType::kImage:
      return fmt::format("Image {}", selection.target_id);
    case SelectionType::kMatch:
      return fmt::format("Match {}", selection.target_id);
    case SelectionType::kPano:
      return fmt::format("Pano {}", selection.target_id);
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

std::ostream& operator<<(std::ostream& oss,
                         const StatusMessage& status_message) {
  return oss << status_message.text << " " << status_message.tooltip;
}

PanoGui::PanoGui(backends::Base* backend, logger::LoggerGui* logger,
                 std::future<utils::Texts> licenses)
    : plot_pane_(backend),
      thumbnail_pane_(backend),
      logger_(logger),
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
  layout_.Begin();
  auto action = DrawSidebar();
  action |= thumbnail_pane_.Draw();
  plot_pane_.Draw(PreviewMessage(selection_));
  if (layout_.ShowDebugInfo()) {
    logger_->Draw();
  }
  about_pane_.Draw();
  return action;
}

Action PanoGui::DrawSidebar() {
  Action action{};
  ImGui::Begin("PanoSweep", nullptr,
               ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar);
  action |= DrawMenu(&compression_options_, &matching_options_);

  DrawWelcomeText();
  ImGui::Separator();

  auto progress = stitcher_pipeline_.LoadingProgress();
  DrawProgressBar(progress);
  if (progress.tasks_done < progress.num_tasks) {
    if (ImGui::Button("Cancel")) {
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
    if (layout_.ShowDebugInfo()) {
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
          pano_future_ = stitcher_pipeline_.RunStitching(
              *stitcher_data_, {.pano_id = selection_.target_id,
                                .export_path = *export_path,
                                .compression = compression_options_});
        }
      }
      break;
    }
    case ActionType::kOpenDirectory:
      [[fallthrough]];
    case ActionType::kOpenFiles: {
      if (auto results = file_dialog::Open(action); !results.empty()) {
        Reset();
        stitcher_data_future_ =
            stitcher_pipeline_.RunLoading(results, {}, matching_options_);
      }
      break;
    }
    case ActionType::kShowMatch: {
      selection_ = {SelectionType::kMatch, action.target_id};
      spdlog::info("Clicked match {}", action.target_id);
      const auto& match = stitcher_data_->matches[action.target_id];
      auto img = DrawMatches(match, stitcher_data_->images);
      plot_pane_.Load(img);
      thumbnail_pane_.SetScrollX(match.id1, match.id2);
      thumbnail_pane_.Highlight(match.id1, match.id2);
      break;
    }
    case ActionType::kShowPano: {
      selection_ = {SelectionType::kPano, action.target_id};
      spdlog::info("Clicked pano {}", selection_.target_id);
      status_message_ = {};
      plot_pane_.Reset();
      pano_future_ = stitcher_pipeline_.RunStitching(
          *stitcher_data_, {.pano_id = selection_.target_id});
      const auto& pano = stitcher_data_->panos[selection_.target_id];
      thumbnail_pane_.SetScrollX(pano.ids);
      thumbnail_pane_.Highlight(pano.ids);
      break;
    }
    case ActionType::kModifyPano: {
      return ModifyPano(action.target_id, &selection_, &stitcher_data_->panos);
    }
    case ActionType::kShowImage: {
      selection_ = {SelectionType::kImage, action.target_id};
      const auto& img = stitcher_data_->images[action.target_id];
      plot_pane_.Load(img.Draw(layout_.ShowDebugInfo()));
      thumbnail_pane_.Highlight(action.target_id);
      break;
    }
    case ActionType::kShowAbout: {
      about_pane_.Show();
      break;
    }
    case ActionType::kToggleDebugLog: {
      layout_.ToggleDebugInfo();
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
        plot_pane_.Load(*result.pano);
      }
    } catch (const std::exception& e) {
      status_message_ = {"Failed to stitch pano", e.what()};
      spdlog::error(status_message_);
      return {};
    }
    if (!result.pano) {
      status_message_ = {
          fmt::format("Failed to stitch pano {}", result.pano_id),
          algorithm::ToString(result.status)};
      spdlog::info(status_message_);
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
  return {};
}

}  // namespace xpano::gui
