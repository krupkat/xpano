// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/utils/imgui_.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <imgui.h>
#include <spdlog/spdlog.h>

#include "xpano/utils/resource.h"

namespace xpano::utils::imgui {

namespace {
constexpr float kDefaultFontSizeBase = 18.0f;
}

bool LoadFonts(const std::filesystem::path& executable_path,
               const FontLoaderArgs& args) {
  std::string alphabet_font_path;
  if (auto font = resource::Find(executable_path, args.alphabet_font_path);
      font) {
    alphabet_font_path = *font;
  } else {
    return false;
  }

  std::string symbols_font_path;
  if (auto font = resource::Find(executable_path, args.symbols_font_path);
      font) {
    symbols_font_path = *font;
  } else {
    return false;
  }

  ImGuiIO& imgui_io = ImGui::GetIO();

  if (const ImFont* alphabet_font = imgui_io.Fonts->AddFontFromFileTTF(
          alphabet_font_path.c_str(), kDefaultFontSizeBase);
      alphabet_font == nullptr) {
    return false;
  }

  ImFontConfig cfg;
  cfg.MergeMode = true;
  if (const ImFont* symbol_font = imgui_io.Fonts->AddFontFromFileTTF(
          symbols_font_path.c_str(), kDefaultFontSizeBase, &cfg);
      symbol_font == nullptr) {
    return false;
  }

  return true;
}

void SetScale(float scale) {
  spdlog::info("Setting scale factor {}x", scale);
  ImGui::GetStyle() = {};
  ImGui::GetStyle().ScaleAllSizes(scale);
  ImGui::GetStyle().FontSizeBase = kDefaultFontSizeBase;
  ImGui::GetStyle().FontScaleDpi = scale;
}

void InfoMarker(const std::string& label, const std::string& desc) {
  ImGui::TextDisabled("%s", label.c_str());
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(desc.c_str());
    ImGui::EndTooltip();
  }
}

std::string InitIniFilePath(
    std::optional<std::filesystem::path> app_data_path) {
  auto ini_file_name = std::string(ImGui::GetIO().IniFilename);
  return app_data_path ? (*app_data_path / ini_file_name).string()
                       : ini_file_name;
}

void DrawScrollableText(const char* label,
                        const std::vector<std::string>& lines, ImVec2 size) {
  ImGui::BeginChild(label, size);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  ImGuiListClipper clipper;
  const int num_lines = static_cast<int>(lines.size());
  clipper.Begin(num_lines);
  while (clipper.Step()) {
    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
      ImGui::TextUnformatted(lines[i].c_str());
    }
  }
  ImGui::PopStyleVar();
  ImGui::EndChild();
}

ImVec2 DpiAwareSize(int width, int height) {
  return {static_cast<float>(width) * ImGui::CalcTextSize("A").x,
          static_cast<float>(height) * ImGui::GetTextLineHeight()};
}

}  // namespace xpano::utils::imgui
