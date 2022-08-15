#include "gui/panels/sidebar.h"

#include <string>
#include <vector>

#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <spdlog/fmt/fmt.h>

#include "algorithm/algorithm.h"
#include "algorithm/image.h"
#include "algorithm/stitcher_pipeline.h"
#include "constants.h"
#include "gui/action.h"
#include "gui/panels/thumbnail_pane.h"

namespace xpano::gui {

namespace {
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

Action DrawFileMenu() {
  Action action{};
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
  return action;
}

Action DrawOptionsMenu(algorithm::CompressionOptions* compression_options) {
  Action action{};
  if (ImGui::BeginMenu("Options")) {
    if (ImGui::BeginMenu("Export compression")) {
      ImGui::SliderInt("JPEG quality", &compression_options->jpeg_quality, 0,
                       kMaxJpegQuality);
      ImGui::Checkbox("JPEG progressive",
                      &compression_options->jpeg_progressive);
      ImGui::Checkbox("JPEG optimize", &compression_options->jpeg_optimize);
      ImGui::SliderInt("PNG compression", &compression_options->png_compression,
                       0, kMaxPngCompression);
      ImGui::EndMenu();
    }
    ImGui::EndMenu();
  }
  return action;
}

Action DrawHelpMenu() {
  Action action{};
  if (ImGui::BeginMenu("Help")) {
    if (ImGui::MenuItem("Show debug info", "CTRL+D")) {
      action |= {ActionType::kToggleDebugLog};
    }
    ImGui::Separator();
    if (ImGui::MenuItem("About")) {
      action |= {ActionType::kShowAbout};
    }
    ImGui::EndMenu();
  }
  return action;
}
}  // namespace

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
  ImGui::TextUnformatted("List of matches:");
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

    if (ImGui::IsItemHovered()) {
      thumbnail_pane.ThumbnailTooltip({matches[i].id1, matches[i].id2});
    }
  }
  ImGui::EndTable();
  return action;
}

Action DrawPanosMenu(const std::vector<algorithm::Pano>& panos,
                     const ThumbnailPane& thumbnail_pane, int highlight_id) {
  ImGui::TextUnformatted("List of panos:");
  ImGui::BeginTable("table2", 3);
  ImGui::TableSetupColumn("Images", ImGuiTableColumnFlags_WidthStretch);
  ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
  ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed);
  ImGui::TableHeadersRow();

  Action action{};

  for (int i = 0; i < panos.size(); i++) {
    ImGui::TableNextColumn();
    auto string = fmt::to_string(fmt::join(panos[i].ids, ","));
    ImGui::Text("%s", string.c_str());
    ImGui::TableNextColumn();
    if (panos[i].exported) {
      ImGui::Text(kCheckMark);
    }
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

Action DrawMenu(algorithm::CompressionOptions* compression_options) {
  Action action{};
  if (ImGui::BeginMenuBar()) {
    action |= DrawFileMenu();
    action |= DrawOptionsMenu(compression_options);
    action |= DrawHelpMenu();
    ImGui::EndMenuBar();
  }
  return action;
}

}  // namespace xpano::gui
