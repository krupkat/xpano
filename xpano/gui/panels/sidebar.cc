#include "xpano/gui/panels/sidebar.h"

#include <algorithm>
#include <string>
#include <vector>

#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <spdlog/fmt/fmt.h>

#include "xpano/algorithm/algorithm.h"
#include "xpano/algorithm/image.h"
#include "xpano/constants.h"
#include "xpano/gui/action.h"
#include "xpano/gui/panels/preview_pane.h"
#include "xpano/gui/panels/thumbnail_pane.h"
#include "xpano/gui/shortcut.h"
#include "xpano/pipeline/stitcher_pipeline.h"
#include "xpano/utils/imgui_.h"
#include "xpano/utils/opencv.h"

namespace xpano::gui {

namespace {
std::string ProgressLabel(pipeline::ProgressType type) {
  switch (type) {
    default:
      return "";
    case pipeline::ProgressType::kLoadingImages:
      return "Loading images";
    case pipeline::ProgressType::kStitchingPano:
      return "Stitching pano";
    case pipeline::ProgressType::kAutoCrop:
      return "Auto crop";
    case pipeline::ProgressType::kDetectingKeypoints:
      return "Detecting keypoints";
    case pipeline::ProgressType::kMatchingImages:
      return "Matching images";
    case pipeline::ProgressType::kExport:
      return "Exporting pano";
    case pipeline::ProgressType::kInpainting:
      return "Auto fill";
  }
}

Action DrawFileMenu() {
  Action action{};
  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Open files", Label(ShortcutType::kOpen))) {
      action |= {ActionType::kOpenFiles};
    }
    if (ImGui::MenuItem("Open directory")) {
      action |= {ActionType::kOpenDirectory};
    }
    if (ImGui::MenuItem("Export", Label(ShortcutType::kExport))) {
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

void DrawCompressionOptionsMenu(
    pipeline::CompressionOptions* compression_options) {
  if (ImGui::BeginMenu("Export compression")) {
    ImGui::Text("JPEG");
    ImGui::SliderInt("Quality", &compression_options->jpeg_quality, 0,
                     kMaxJpegQuality);
    ImGui::Checkbox("Progressive", &compression_options->jpeg_progressive);
    ImGui::Checkbox("Optimize", &compression_options->jpeg_optimize);
    ImGui::Text("Chroma subsampling:");
    ImGui::SameLine();
    if constexpr (utils::opencv::HasJpegSubsamplingSupport()) {
      utils::imgui::RadioBox(&compression_options->jpeg_subsampling,
                             pipeline::kSubsamplingModes);
      utils::imgui::InfoMarker("(?)",
                               "Corresponding to the 4:4:4, 4:2:2 and 4:2:0 "
                               "chroma subsampling modes.");
    }
    ImGui::Separator();
    ImGui::Text("PNG");
    ImGui::SliderInt("Compression", &compression_options->png_compression, 0,
                     kMaxPngCompression);
    ImGui::EndMenu();
  }
}

void DrawLoadingOptionsMenu(pipeline::LoadingOptions* loading_options) {
  if (ImGui::BeginMenu("Image loading")) {
    ImGui::Text(
        "Modify this for faster image loading / more precision in panorama "
        "detection.");
    ImGui::Spacing();
    if (ImGui::InputInt("Preview size", &loading_options->preview_longer_side,
                        kStepPreviewLongerSide, kStepPreviewLongerSide)) {
      loading_options->preview_longer_side =
          std::max(loading_options->preview_longer_side, kMinPreviewLongerSide);
      loading_options->preview_longer_side =
          std::min(loading_options->preview_longer_side, kMaxPreviewLongerSide);
    }
    ImGui::SameLine();
    utils::imgui::InfoMarker(
        "(?)",
        "Size of the preview image's longer side in pixels.\n - decrease to "
        "get faster loading times.\n - increase to get nicer preview images\n "
        "- increase to get more precision for panorama detection.");
    ImGui::EndMenu();
  }
}

bool DrawMatchConf(float* match_conf) {
  bool value_changed = false;
  if (ImGui::InputFloat("Match confidence", match_conf, 0.01f, 0.01f, "%.2f")) {
    value_changed = true;

    if (*match_conf < kMinMatchConf) {
      *match_conf = kMinMatchConf;
    }

    if (*match_conf > kMaxMatchConf) {
      *match_conf = kMaxMatchConf;
    }
  }
  ImGui::SameLine();
  utils::imgui::InfoMarker(
      "(?)",
      "Increasing this value will get less matches, but with higher quality."
      "\nChanging both ways can be useful in case a panorama fails to detect "
      "or stitch.");
  return value_changed;
}

void DrawMatchingOptionsMenu(pipeline::MatchingOptions* matching_options,
                             bool debug_enabled) {
  if (ImGui::BeginMenu("Panorama detection")) {
    ImGui::Text(
        "Experiment with this if the app cannot find the panoramas you "
        "want.");
    ImGui::Spacing();
    ImGui::SliderInt("Matching distance",
                     &matching_options->neighborhood_search_size, 0,
                     kMaxNeighborhoodSearchSize);
    ImGui::SameLine();
    utils::imgui::InfoMarker(
        "(?)",
        "Select how many neighboring images will be considered for panorama "
        "auto detection.");
    ImGui::SliderInt("Matching threshold", &matching_options->match_threshold,
                     kMinMatchThreshold, kMaxMatchThreshold);
    ImGui::SameLine();
    utils::imgui::InfoMarker(
        "(?)",
        "Number of keypoints that need to match in order to include the two "
        "images in a panorama.");
    if (debug_enabled) {
      ImGui::SeparatorText("Debug");
      DrawMatchConf(&matching_options->match_conf);
    }
    ImGui::EndMenu();
  }
}

Action DrawProjectionOptions(pipeline::StitchAlgorithmOptions* stitch_options) {
  Action action{};
  ImGui::Text("Projection type:");
  ImGui::SameLine();
  utils::imgui::InfoMarker(
      "(?)", "Projection types marked with a star are experimental.");
  ImGui::Spacing();
  if (utils::imgui::ComboBox(&stitch_options->projection.type,
                             algorithm::kProjectionTypes,
                             "##projection_type")) {
    action |= {ActionType::kRecomputePano};
  }

  if (algorithm::HasAdvancedParameters(stitch_options->projection.type)) {
    ImGui::Text("Advanced projection parameters:");
    ImGui::Spacing();
    if (ImGui::InputFloat("a", &stitch_options->projection.a_param, 0.5f,
                          0.5f)) {
      action |= {ActionType::kRecomputePano};
    }
    if (ImGui::InputFloat("b", &stitch_options->projection.b_param, 0.5f,
                          0.5f)) {
      action |= {ActionType::kRecomputePano};
    }
  }
  return action;
}

Action DrawFeatureMatchingOptions(
    pipeline::StitchAlgorithmOptions* stitch_options) {
  Action action{};
  ImGui::Text("Feature algorithm for matching:");
  ImGui::Spacing();
  if (utils::imgui::ComboBox(&stitch_options->feature, algorithm::kFeatureTypes,
                             "##feature_type")) {
    action |= {ActionType::kRecomputePano};
  }
  return action;
}

Action DrawWaveCorrectionOptions(
    pipeline::StitchAlgorithmOptions* stitch_options) {
  Action action{};
  ImGui::Text("Wave correction:");
  ImGui::SameLine();
  utils::imgui::InfoMarker(
      "(?)",
      "Applies a correction to straighten the panorama. Can be turned off "
      "completely.\nThe auto option will estimate if the panorama is "
      "horizontal or vertical.");
  ImGui::Spacing();
  if (utils::imgui::ComboBox(&stitch_options->wave_correction,
                             algorithm::kWaveCorrectionTypes,
                             "##wave_correction_type")) {
    action |= {ActionType::kRecomputePano};
  }
  return action;
}

Action DrawStitchOptionsMenu(pipeline::StitchAlgorithmOptions* stitch_options,
                             bool debug_enabled) {
  Action action{};
  if (ImGui::BeginMenu("Panorama stitching")) {
    action |= DrawProjectionOptions(stitch_options);
    action |= DrawWaveCorrectionOptions(stitch_options);

    if (debug_enabled) {
      ImGui::SeparatorText("Debug");
      action |= DrawFeatureMatchingOptions(stitch_options);

      if (DrawMatchConf(&stitch_options->match_conf)) {
        action |= {ActionType::kRecomputePano};
      }
    }
    ImGui::EndMenu();
  }
  return action;
}

void DrawAutofillOptionsMenu(pipeline::InpaintingOptions* inpaint_options) {
  if (ImGui::BeginMenu("Auto fill")) {
    ImGui::SeparatorText("Debug");
    ImGui::Text("Algorithm:");
    ImGui::Spacing();
    utils::imgui::ComboBox(&inpaint_options->method,
                           algorithm::kInpaintingMethods, "##inpaint_type");
    ImGui::Text("Advanced algorithm parameters:");
    ImGui::Spacing();
    if (ImGui::InputDouble("Radius", &inpaint_options->radius,
                           kInpaintingRadiusStep, kInpaintingRadiusStep)) {
      inpaint_options->radius =
          std::clamp(inpaint_options->radius, kDefaultInpaintingRadius,
                     kMaxInpaintingRadius);
    }
    ImGui::EndMenu();
  }
}

Action DrawOptionsMenu(pipeline::CompressionOptions* compression_options,
                       pipeline::LoadingOptions* loading_options,
                       pipeline::InpaintingOptions* inpaint_options,
                       pipeline::MatchingOptions* matching_options,
                       pipeline::StitchAlgorithmOptions* stitch_options,
                       bool debug_enabled) {
  Action action{};
  if (ImGui::BeginMenu("Options")) {
    DrawCompressionOptionsMenu(compression_options);
    DrawLoadingOptionsMenu(loading_options);
    DrawMatchingOptionsMenu(matching_options, debug_enabled);
    action |= DrawStitchOptionsMenu(stitch_options, debug_enabled);
    if (debug_enabled) {
      DrawAutofillOptionsMenu(inpaint_options);
    }
    ImGui::EndMenu();
  }
  return action;
}

Action DrawHelpMenu() {
  Action action{};
  if (ImGui::BeginMenu("Help")) {
    if (ImGui::MenuItem("Show debug info", Label(ShortcutType::kDebug))) {
      action |= {ActionType::kToggleDebugLog};
    }
    if (ImGui::MenuItem("Support")) {
      action |= {ActionType::kShowBugReport};
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

void DrawProgressBar(pipeline::ProgressReport progress) {
  if (progress.num_tasks == 0) {
    return;
  }
  const int max_percent = 100;
  float progress_ratio = static_cast<float>(progress.tasks_done) /
                         static_cast<float>(progress.num_tasks);
  std::string label =
      progress.tasks_done == progress.num_tasks
          ? "100%"
          : fmt::format("{}: {:.0f}%", ProgressLabel(progress.type),
                        progress_ratio * max_percent);
  ImGui::ProgressBar(progress_ratio, ImVec2(-1.0f, 0.f), label.c_str());
}

cv::Mat DrawMatches(const algorithm::Match& match,
                    const std::vector<algorithm::Image>& images) {
  cv::Mat out;
  const auto& img1 = images[match.id1];
  const auto& img2 = images[match.id2];
  const int match_thickness = 1;
  const auto match_color = cv::Scalar(0, 255, 0);
  const auto single_point_color = cv::Scalar::all(-1);
  const auto matches_mask = std::vector<char>();
  cv::drawMatches(img1.GetPreview(), img1.GetKeypoints(), img2.GetPreview(),
                  img2.GetKeypoints(), match.matches, out, match_thickness,
                  match_color, single_point_color, matches_mask,
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
    ImGui::Text("%ld", matches[i].matches.size());
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
    auto string = fmt::format("{}", fmt::join(panos[i].ids, ","));
    ImGui::TextUnformatted(string.c_str());
    ImGui::TableNextColumn();
    if (panos[i].exported) {
      ImGui::TextUnformatted(kCheckMark);
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

Action DrawMenu(pipeline::CompressionOptions* compression_options,
                pipeline::LoadingOptions* loading_options,
                pipeline::InpaintingOptions* inpaint_options,
                pipeline::MatchingOptions* matching_options,
                pipeline::StitchAlgorithmOptions* stitch_options,
                bool debug_enabled) {
  Action action{};
  if (ImGui::BeginMenuBar()) {
    action |= DrawFileMenu();
    action |=
        DrawOptionsMenu(compression_options, loading_options, inpaint_options,
                        matching_options, stitch_options, debug_enabled);
    action |= DrawHelpMenu();
    ImGui::EndMenuBar();
  }
  return action;
}

void DrawWelcomeTextPart1() {
  ImGui::Text("Welcome to Xpano!");
  ImGui::Text(" 1) Import your images:");
}

void DrawWelcomeTextPart2() {
  ImGui::Text(" 2) Select a panorama");
  ImGui::SameLine();
  utils::imgui::InfoMarker(
      "(?)",
      "a) Pick one of the autodetected panoramas\nb) CTRL click on thumbnails "
      "to add / edit / delete panoramas\nc) Zoom and pan the images with your "
      "mouse");
  ImGui::Text(" 3) Available actions:");
  ImGui::SameLine();
  utils::imgui::InfoMarker(
      "(?)",
      "a) Select projection type\nb) Compute full resolution panorama "
      "preview\nc) Toggle crop mode\nd) Auto fill empty space in the "
      "panorama\ne) Panorama export\n - Works either with preview or full "
      "resolution panoramas\n - In both cases exports a full resolution "
      "panorama");
  ImGui::Spacing();
}

Action DrawImportActionButtons() {
  Action action{};
  ImGui::Spacing();
  if (ImGui::Button("Multiple files")) {
    action |= {ActionType::kOpenFiles};
  }
  ImGui::SameLine();
  if (ImGui::Button("Directory")) {
    action |= {ActionType::kOpenDirectory};
  }
  return action;
};

Action DrawActionButtons(ImageType image_type, int target_id,
                         algorithm::ProjectionType* projection_type) {
  Action action{};
  if (utils::imgui::ComboBox(projection_type, algorithm::kProjectionTypes,
                             "##projection_type", ImGuiComboFlags_NoPreview)) {
    action |= {ActionType::kRecomputePano};
  }
  ImGui::SameLine();
  utils::imgui::EnableIf(
      image_type == ImageType::kPanoPreview,
      [&] {
        if (ImGui::Button("Full-res")) {
          action |=
              {.type = ActionType::kShowFullResPano, .target_id = target_id};
        }
      },
      image_type == ImageType::kPanoFullRes ? "Already computed"
                                            : "First select a panorama");
  ImGui::SameLine();
  utils::imgui::EnableIf(
      image_type == ImageType::kPanoFullRes,
      [&] {
        if (ImGui::Button("Crop")) {
          action |= {ActionType::kToggleCrop};
        }
      },
      "First compute a full resolution panorama");
  ImGui::SameLine();
  utils::imgui::EnableIf(
      image_type == ImageType::kPanoFullRes,
      [&] {
        if (ImGui::Button("Fill")) {
          action |= {ActionType::kInpaint};
        }
      },
      "First compute a full resolution panorama");
  ImGui::SameLine();
  utils::imgui::EnableIf(
      image_type == ImageType::kPanoFullRes ||
          image_type == ImageType::kPanoPreview,
      [&] {
        if (ImGui::Button("Export")) {
          action |= {ActionType::kExport};
        }
      },
      "First select a panorama");
  ImGui::Spacing();
  return action;
}

}  // namespace xpano::gui
