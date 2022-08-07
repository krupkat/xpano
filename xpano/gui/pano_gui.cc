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
#include <SDL.h>
#include <spdlog/fmt/fmt.h>

#include "algorithm/algorithm.h"
#include "algorithm/image.h"
#include "algorithm/stitcher_pipeline.h"
#include "gui/action.h"
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

void DrawProgressBar(float progress) {
  int progress_int = static_cast<int>(progress * 100);
  std::string label = fmt::format("{}%", progress_int);
  ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.f), label.c_str());
}

cv::Mat DrawMatches(const algorithm::Match& match,
                    const std::vector<algorithm::Image>& images) {
  cv::Mat out;
  const auto& img1 = images[match.id1];
  const auto& img2 = images[match.id2];
  cv::drawMatches(img1.GetImageData(), img1.GetKeypoints(), img2.GetImageData(),
                  img2.GetKeypoints(), match.matches, out, 1,
                  cv::Scalar(0, 255, 0), cv::Scalar::all(-1),
                  std::vector<char>(),
                  cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

  return out;
}

Action DrawMatchesMenu(const std::vector<algorithm::Match>& matches,
                       int highlight_id) {
  ImGui::Separator();
  ImGui::BeginTable("table1", 3);

  ImGui::TableSetupColumn("Matched");
  ImGui::TableSetupColumn("Inliers");
  ImGui::TableSetupColumn("Action");
  ImGui::TableHeadersRow();

  Action action{};

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
  }

  ImGui::EndTable();
  return action;
}

Action DrawPanosMenu(const std::vector<algorithm::Pano>& panos,
                     int highlight_id) {
  ImGui::Separator();
  ImGui::BeginTable("table2", 2);

  ImGui::TableSetupColumn("Pano");
  ImGui::TableSetupColumn("Action");
  ImGui::TableHeadersRow();

  Action action{};

  for (int i = 0; i < panos.size(); i++) {
    ImGui::TableNextColumn();
    auto string = fmt::format("{}", fmt::join(panos[i].ids, ","));
    ImGui::Text("%s", string.c_str());
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
  }

  ImGui::EndTable();
  return action;
}

Action CheckKeybindings() {
  if (ImGui::IsKeyReleased(ImGuiKey_D)) {
    return {ActionType::kToggleDebugLog};
  }
  return {ActionType::kNone};
}

}  // namespace

PanoGui::PanoGui(SDL_Renderer* renderer, logger::LoggerGui* logger)
    : plot_pane_(renderer), thumbnail_pane_(renderer), logger_(logger) {}

void PanoGui::Run() {
  auto action = DrawGui();
  action |= CheckKeybindings();
  if (action.type != ActionType::kNone) {
    ResetSelections(action);
    PerformAction(action);
  }
  ResolveFutures();
}

Action PanoGui::DrawGui() {
  layout_.Begin();
  auto action = DrawSidebar();
  action |= thumbnail_pane_.Draw();
  plot_pane_.Draw();
  if (layout_.ShowLogger()) {
    logger_->Draw();
  }
  return action;
}

Action PanoGui::DrawSidebar() {
  ImGui::Begin("PanoSweep");
  ImGui::Text("Welcome to PanoSweep");
  Action action{};

  if (ImGui::Button("Open")) {
    action |= {ActionType::kOpenFiles};
  }

  if (float progress = stitcher_pipeline_.LoadingProgress(); progress > 0.0f) {
    DrawProgressBar(progress);
  }

  if (stitcher_data_) {
    action |= DrawPanosMenu(stitcher_data_->panos, selected_pano_);
    action |= DrawMatchesMenu(stitcher_data_->matches, selected_match_);
  }
  ImGui::End();
  return action;
}

void PanoGui::ResetSelections(Action action) {
  if (action.type == ActionType::kModifyPano ||
      action.type == ActionType::kToggleDebugLog) {
    return;
  }

  selected_pano_ = -1;
  selected_match_ = -1;
}

void PanoGui::PerformAction(Action action) {
  switch (action.type) {
    case ActionType::kNone: {
      break;
    }
    case ActionType::kOpenFiles: {
      auto results = file_dialog::CallNfd();
      stitcher_data_.reset();
      thumbnail_pane_.Reset();
      stitcher_data_future_ = stitcher_pipeline_.RunLoading(results, {});
      break;
    }
    case ActionType::kShowImage: {
      const auto& img = stitcher_data_->images[action.id];
      plot_pane_.Load(img.Draw());
      thumbnail_pane_.Highlight(action.id);
      break;
    }
    case ActionType::kShowMatch: {
      selected_match_ = action.id;
      SDL_Log("Clicked match %d", action.id);
      const auto& match = stitcher_data_->matches[action.id];
      auto img = DrawMatches(match, stitcher_data_->images);
      plot_pane_.Load(img);
      thumbnail_pane_.SetScrollX(match.id1, match.id2);
      thumbnail_pane_.Highlight(match.id1, match.id2);
      break;
    }
    case ActionType::kShowPano: {
      selected_pano_ = action.id;
      SDL_Log("Clicked pano %d", action.id);
      pano_future_ =
          stitcher_pipeline_.RunStitching(*stitcher_data_, action.id);
      const auto& pano = stitcher_data_->panos[selected_pano_];
      thumbnail_pane_.SetScrollX(pano.ids);
      thumbnail_pane_.Highlight(pano.ids, true);
      break;
    }
    case ActionType::kModifyPano: {
      auto& pano = stitcher_data_->panos[selected_pano_];
      auto iter = std::find(pano.ids.begin(), pano.ids.end(), action.id);
      if (iter == pano.ids.end()) {
        pano.ids.push_back(action.id);
      } else {
        pano.ids.erase(iter);
      }
      thumbnail_pane_.Highlight(pano.ids, true);
      break;
    }
    case ActionType::kToggleDebugLog: {
      layout_.ToggleLogger();
      break;
    }
  }
}

void PanoGui::ResolveFutures() {
  if (IsReady(stitcher_data_future_)) {
    stitcher_data_ = stitcher_data_future_.get();
    thumbnail_pane_.Load(stitcher_data_->images);
  }

  if (IsReady(pano_future_)) {
    auto pano = pano_future_.get();
    SDL_Log("Received pano");
    if (pano) {
      plot_pane_.Load(*pano);
    }
  }
}

}  // namespace xpano::gui
