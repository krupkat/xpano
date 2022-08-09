#include "gui/pano_gui.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <optional>
#include <string>
#include <vector>

#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
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
#include "gui/preview_pane.h"
#include "gui/thumbnail_pane.h"
#include "log/logger.h"

namespace xpano::gui {

namespace {

template <typename TType>
bool IsReady(const std::future<TType>& future) {
  return future.valid() &&
         future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

std::string ProgressLabel(algorithm::ProgressType type) {
  switch (type) {
    default:
      return "";
    case algorithm::ProgressType::kLoadingImages:
      return "Loading images";
    case algorithm::ProgressType::kStitchingPano:
      return "Stitching pano";
    case algorithm::ProgressType::kDetectingKeypoints:
      return "Detecting keypoints";
    case algorithm::ProgressType::kMatchingImages:
      return "Matching images";
  }
}

void DrawProgressBar(algorithm::ProgressReport progress) {
  if (progress.num_tasks == 0) {
    return;
  }
  int percentage = progress.tasks_done * 100 / progress.num_tasks;
  std::string label =
      progress.tasks_done == progress.num_tasks
          ? "100%"
          : fmt::format("{}: {}%", ProgressLabel(progress.type), percentage);
  ImGui::ProgressBar(static_cast<float>(percentage) / 100.0f,
                     ImVec2(-1.0f, 0.f), label.c_str());
}

cv::Mat DrawMatches(const algorithm::Match& match,
                    const std::vector<algorithm::Image>& images) {
  cv::Mat out;
  const auto& img1 = images[match.id1];
  const auto& img2 = images[match.id2];
  cv::drawMatches(img1.GetPreview(), img1.GetKeypoints(), img2.GetPreview(),
                  img2.GetKeypoints(), match.matches, out, 1,
                  cv::Scalar(0, 255, 0), cv::Scalar::all(-1),
                  std::vector<char>(),
                  cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

  return out;
}

Action DrawMatchesMenu(const std::vector<algorithm::Match>& matches,
                       const ThumbnailPane& thumbnail_pane, int highlight_id) {
  Action action{};
  ImGui::BeginTable("table1", 3);
  ImGui::TableSetupColumn("Matched");
  ImGui::TableSetupColumn("Inliers");
  ImGui::TableSetupColumn("Action");
  ImGui::TableHeadersRow();

  for (int i = 0; i < matches.size(); i++) {
    ImGui::TableNextColumn();
    ImGui::Text("%d, %d", matches[i].id1, matches[i].id2);
    ImGui::TableNextColumn();
    ImGui::Text("%d", matches[i].matches.size());
    ImGui::TableNextColumn();
    ImGui::PushID(i);
    if (ImGui::SmallButton("Show")) {
      action = {ActionType::kShowMatch, i};
    }
    ImGui::PopID();

    if (i == highlight_id || ImGui::IsItemHovered()) {
      ImU32 row_bg_color = ImGui::GetColorU32(ImGuiCol_TableRowBgAlt);
      ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row_bg_color);
    }

    if (ImGui::IsItemHovered()) {
      thumbnail_pane.ThumbnailTooltip({matches[i].id1, matches[i].id2});
    }
  }
  ImGui::EndTable();
  return action;
}

Action DrawPanosMenu(const std::vector<algorithm::Pano>& panos,
                     const ThumbnailPane& thumbnail_pane, int highlight_id) {
  ImGui::BeginTable("table2", 3);
  ImGui::TableSetupColumn("Images", ImGuiTableColumnFlags_WidthStretch);
  ImGui::TableSetupColumn("Done", ImGuiTableColumnFlags_WidthFixed);
  ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed);
  ImGui::TableHeadersRow();

  Action action{};

  for (int i = 0; i < panos.size(); i++) {
    ImGui::TableNextColumn();
    auto string = fmt::to_string(fmt::join(panos[i].ids, ","));
    ImGui::Text("%s", string.c_str());
    ImGui::TableNextColumn();
    ImGui::Text(panos[i].exported ? "x" : " ");
    ImGui::TableNextColumn();
    ImGui::PushID(i);
    if (ImGui::SmallButton("Show")) {
      action = {ActionType::kShowPano, i};
    }
    ImGui::PopID();

    if (i == highlight_id || ImGui::IsItemHovered()) {
      ImU32 row_bg_color = ImGui::GetColorU32(ImGuiCol_TableRowBgAlt);
      ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row_bg_color);
    }

    if (ImGui::IsItemHovered()) {
      thumbnail_pane.ThumbnailTooltip(panos[i].ids);
    }
  }
  ImGui::EndTable();
  return action;
}

Action DrawMenu() {
  Action action{};
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Open files", "CTRL+O")) {
        action |= {ActionType::kOpenFiles};
      }
      if (ImGui::MenuItem("Open directory")) {
        action |= {ActionType::kOpenDirectory};
      }
      if (ImGui::MenuItem("Export", "CTRL+S")) {
        action |= {ActionType::kExport};
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Quit")) {
        action |= {ActionType::kQuit};
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Options")) {
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      if (ImGui::MenuItem("Show debug info", "CTRL+D")) {
        action |= {ActionType::kToggleDebugLog};
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
  return action;
}

Action CheckKeybindings() {
  bool ctrl = ImGui::GetIO().KeyCtrl;
  if (ctrl && ImGui::IsKeyReleased(ImGuiKey_O)) {
    return {ActionType::kOpenFiles};
  }
  if (ctrl && ImGui::IsKeyReleased(ImGuiKey_S)) {
    return {ActionType::kExport};
  }
  if (ctrl && ImGui::IsKeyReleased(ImGuiKey_D)) {
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
        pano_future_ = stitcher_pipeline_.RunStitching(
            *stitcher_data_, {.pano_id = selected_pano_,
                              .full_res = true,
                              .export_path = "export.jpg"});
      }
      break;
    }
    case ActionType::kOpenDirectory:
      [[fallthrough]];
    case ActionType::kOpenFiles: {
      if (auto results = file_dialog::CallNfd(action); !results.empty()) {
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
