#include "gui/pano_gui.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <optional>
#include <string>
#include <vector>

#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <spdlog/fmt/fmt.h>
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

namespace xpano::gui {

namespace {

template <typename TType>
bool IsReady(const std::future<TType>& future) {
  return future.valid() &&
         future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

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

}  // namespace

PanoGui::PanoGui(backends::Base* backend, logger::LoggerGui* logger)
    : plot_pane_(backend), thumbnail_pane_(backend), logger_(logger) {}

bool PanoGui::Run() {
  auto action = DrawGui();
  action |= CheckKeybindings();
  if (action.type != ActionType::kNone) {
    ResetSelections(action);
    PerformAction(action);
  }
  ResolveFutures();
  return action.type == ActionType::kQuit;
}

Action PanoGui::DrawGui() {
  layout_.Begin();
  auto action = DrawSidebar();
  action |= thumbnail_pane_.Draw();
  plot_pane_.Draw();
  if (layout_.ShowDebugInfo()) {
    logger_->Draw();
  }
  return action;
}

Action PanoGui::DrawSidebar() {
  Action action{};
  ImGui::Begin("PanoSweep", nullptr, ImGuiWindowFlags_MenuBar);
  action |= DrawMenu();

  ImGui::Text("Welcome to PanoSweep");
  DrawProgressBar(stitcher_pipeline_.LoadingProgress());
  ImGui::Text("%s ", info_message_.c_str());
  if (stitcher_data_) {
    action |=
        DrawPanosMenu(stitcher_data_->panos, thumbnail_pane_, selected_pano_);
    if (layout_.ShowDebugInfo()) {
      action |= DrawMatchesMenu(stitcher_data_->matches, thumbnail_pane_,
                                selected_match_);
    }
  }
  ImGui::End();
  return action;
}

void PanoGui::ResetSelections(Action action) {
  if (action.type == ActionType::kModifyPano ||
      action.type == ActionType::kToggleDebugLog ||
      action.type == ActionType::kExport) {
    return;
  }

  selected_image_ = -1;
  selected_pano_ = -1;
  selected_match_ = -1;
}

void PanoGui::ModifyPano(Action action) {
  // Pano is being edited
  if (selected_pano_ >= 0) {
    auto& pano = stitcher_data_->panos[selected_pano_];
    auto iter = std::find(pano.ids.begin(), pano.ids.end(), action.id);
    if (iter == pano.ids.end()) {
      pano.ids.push_back(action.id);
    } else {
      pano.ids.erase(iter);
    }
    // Pano was deleted
    if (pano.ids.empty()) {
      auto pano_iter = stitcher_data_->panos.begin() + selected_pano_;
      stitcher_data_->panos.erase(pano_iter);
      selected_pano_ = -1;
      thumbnail_pane_.DisableHighlight();
    } else {
      thumbnail_pane_.Highlight(pano.ids);
    }
  }

  if (selected_image_ >= 0) {
    // Deselect image
    if (selected_image_ == action.id) {
      selected_image_ = -1;
      thumbnail_pane_.DisableHighlight();
      return;
    }

    // Start a new pano from selected image
    auto new_pano =
        algorithm::Pano{std::vector<int>({selected_image_, action.id})};
    stitcher_data_->panos.push_back(new_pano);
    thumbnail_pane_.Highlight(new_pano.ids);
    selected_pano_ = static_cast<int>(stitcher_data_->panos.size()) - 1;
    selected_image_ = -1;
  }

  // Queue pano stitching
  if (selected_pano_ >= 0) {
    pano_future_ = stitcher_pipeline_.RunStitching(*stitcher_data_,
                                                   {.pano_id = selected_pano_});
  }
}

void PanoGui::PerformAction(Action action) {
  switch (action.type) {
    default: {
      break;
    }
    case ActionType::kExport: {
      if (selected_pano_ >= 0) {
        spdlog::info("Exporting pano {}", selected_pano_);
        info_message_.clear();
        auto default_name = fmt::format("pano_{}.jpg", selected_pano_);
        auto export_path = file_dialog::Save(default_name);
        if (export_path) {
          pano_future_ = stitcher_pipeline_.RunStitching(
              *stitcher_data_, {.pano_id = selected_pano_,
                                .full_res = true,
                                .export_path = *export_path});
        }
      }
      break;
    }
    case ActionType::kOpenDirectory:
      [[fallthrough]];
    case ActionType::kOpenFiles: {
      if (auto results = file_dialog::Open(action); !results.empty()) {
        stitcher_data_.reset();
        thumbnail_pane_.Reset();
        info_message_.clear();
        stitcher_data_future_ = stitcher_pipeline_.RunLoading(results, {});
      }
      break;
    }
    case ActionType::kShowMatch: {
      selected_match_ = action.id;
      spdlog::info("Clicked match {}", action.id);
      const auto& match = stitcher_data_->matches[action.id];
      auto img = DrawMatches(match, stitcher_data_->images);
      plot_pane_.Load(img);
      thumbnail_pane_.SetScrollX(match.id1, match.id2);
      thumbnail_pane_.Highlight(match.id1, match.id2);
      break;
    }
    case ActionType::kShowPano: {
      selected_pano_ = action.id;
      spdlog::info("Clicked pano {}", selected_pano_);
      info_message_.clear();
      pano_future_ = stitcher_pipeline_.RunStitching(
          *stitcher_data_, {.pano_id = selected_pano_});
      const auto& pano = stitcher_data_->panos[selected_pano_];
      thumbnail_pane_.SetScrollX(pano.ids);
      thumbnail_pane_.Highlight(pano.ids);
      break;
    }
    case ActionType::kModifyPano: {
      if (selected_image_ >= 0 || selected_pano_ >= 0 || selected_match_ >= 0) {
        ModifyPano(action);
        break;
      }
      [[fallthrough]];
    }
    case ActionType::kShowImage: {
      selected_image_ = action.id;
      const auto& img = stitcher_data_->images[action.id];
      plot_pane_.Load(img.Draw(layout_.ShowDebugInfo()));
      thumbnail_pane_.Highlight(action.id);
      break;
    }
    case ActionType::kToggleDebugLog: {
      layout_.ToggleDebugInfo();
      break;
    }
  }
}

void PanoGui::ResolveFutures() {
  if (IsReady(stitcher_data_future_)) {
    stitcher_data_ = stitcher_data_future_.get();
    thumbnail_pane_.Load(stitcher_data_->images);
    info_message_ =
        fmt::format("Loaded {} images", stitcher_data_->images.size());
  }
  if (IsReady(pano_future_)) {
    auto result = pano_future_.get();
    spdlog::info("Received pano");
    if (result.pano) {
      plot_pane_.Load(*result.pano);
      info_message_ = fmt::format("Stitched pano successfully");

      if (result.options.full_res) {
        cv::imwrite(result.options.export_path, *result.pano);
        info_message_ =
            fmt::format("Pano exported to {}", result.options.export_path);
        stitcher_data_->panos[result.options.pano_id].exported = true;
      }
    } else {
      info_message_ = "Failed to stitch pano";
    }
  }
}

}  // namespace xpano::gui
