#include "gui/pano_gui.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <optional>
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
  ImGui::Text("%s", status_message.basic.c_str());
  if (!status_message.details.empty()) {
    ImGui::SameLine();
    utils::imgui::InfoMarker("(info)", status_message.details);
  }
}

}  // namespace

std::ostream& operator<<(std::ostream& oss,
                         const StatusMessage& status_message) {
  return oss << status_message.basic << " " << status_message.details;
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
    ResetSelections(action);
    action |= PerformAction(action);
  }

  if (action.delayed) {
    delayed_action_ = RemoveDelay(action);
  }
  return action.type == ActionType::kQuit;
}

std::string PanoGui::PreviewMessage() const {
  if (selected_image_ >= 0) {
    return fmt::format("Image {}", selected_image_);
  }
  if (selected_pano_ >= 0) {
    return fmt::format("Pano {}", selected_pano_);
  }
  return "";
}

Action PanoGui::DrawGui() {
  layout_.Begin();
  auto action = DrawSidebar();
  action |= thumbnail_pane_.Draw();
  plot_pane_.Draw(PreviewMessage());
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
  action |= DrawMenu(&compression_options_);

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
    action |=
        DrawPanosMenu(stitcher_data_->panos, thumbnail_pane_, selected_pano_);
    if (layout_.ShowDebugInfo()) {
      action |= DrawMatchesMenu(stitcher_data_->matches, thumbnail_pane_,
                                selected_match_);
    }
  }

  ImGui::EndChild();
  ImGui::End();
  return action;
}

void PanoGui::ResetSelections(Action action) {
  if (action.type == ActionType::kModifyPano ||
      action.type == ActionType::kToggleDebugLog ||
      action.type == ActionType::kExport ||
      action.type == ActionType::kShowAbout) {
    return;
  }

  selected_image_ = -1;
  selected_pano_ = -1;
  selected_match_ = -1;
}

Action PanoGui::ModifyPano(Action action) {
  // Pano is being edited
  if (selected_pano_ >= 0) {
    auto& pano = stitcher_data_->panos[selected_pano_];
    auto iter = std::find(pano.ids.begin(), pano.ids.end(), action.target_id);
    if (iter == pano.ids.end()) {
      pano.ids.push_back(action.target_id);
    } else {
      pano.ids.erase(iter);
    }
    // Pano was deleted
    if (pano.ids.empty()) {
      auto pano_iter = stitcher_data_->panos.begin() + selected_pano_;
      stitcher_data_->panos.erase(pano_iter);
      selected_pano_ = -1;
      thumbnail_pane_.DisableHighlight();
    }
  }

  if (selected_image_ >= 0) {
    // Deselect image
    if (selected_image_ == action.target_id) {
      selected_image_ = -1;
      thumbnail_pane_.DisableHighlight();
      return {};
    }

    // Start a new pano from selected image
    selected_pano_ = static_cast<int>(stitcher_data_->panos.size());
    stitcher_data_->panos.push_back(
        {.ids = {selected_image_, action.target_id}});
    selected_image_ = -1;
  }

  // Queue pano stitching
  if (selected_pano_ >= 0) {
    return {.type = ActionType::kShowPano,
            .target_id = selected_pano_,
            .delayed = true};
  }
  return {};
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
    case ActionType::kExport: {
      if (selected_pano_ >= 0) {
        spdlog::info("Exporting pano {}", selected_pano_);
        status_message_ = {};
        auto default_name = fmt::format("pano_{}.jpg", selected_pano_);
        auto export_path = file_dialog::Save(default_name);
        if (export_path) {
          pano_future_ = stitcher_pipeline_.RunStitching(
              *stitcher_data_, {.pano_id = selected_pano_,
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
        thumbnail_pane_.Reset();
        plot_pane_.Reset();
        status_message_ = {};
        stitcher_pipeline_.Cancel();
        stitcher_data_.reset();
        stitcher_data_future_ = stitcher_pipeline_.RunLoading(results, {});
      }
      break;
    }
    case ActionType::kShowMatch: {
      selected_match_ = action.target_id;
      spdlog::info("Clicked match {}", action.target_id);
      const auto& match = stitcher_data_->matches[action.target_id];
      auto img = DrawMatches(match, stitcher_data_->images);
      plot_pane_.Load(img);
      thumbnail_pane_.SetScrollX(match.id1, match.id2);
      thumbnail_pane_.Highlight(match.id1, match.id2);
      break;
    }
    case ActionType::kShowPano: {
      selected_pano_ = action.target_id;
      spdlog::info("Clicked pano {}", selected_pano_);
      status_message_ = {};
      plot_pane_.Reset();
      pano_future_ = stitcher_pipeline_.RunStitching(
          *stitcher_data_, {.pano_id = selected_pano_});
      const auto& pano = stitcher_data_->panos[selected_pano_];
      thumbnail_pane_.SetScrollX(pano.ids);
      thumbnail_pane_.Highlight(pano.ids);
      break;
    }
    case ActionType::kModifyPano: {
      if (selected_image_ >= 0 || selected_pano_ >= 0 || selected_match_ >= 0) {
        return ModifyPano(action);
      }
      [[fallthrough]];
    }
    case ActionType::kShowImage: {
      selected_image_ = action.target_id;
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
