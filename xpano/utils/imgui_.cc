// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/utils/imgui_.h"

#include <cmath>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_impl_sdlrenderer3.h>

#include "xpano/utils/resource.h"

namespace xpano::utils::imgui {

FontLoader::FontLoader(const FontLoaderArgs& args)
    : alphabet_font_path_(args.alphabet_font_path),
      symbols_font_path_(args.symbols_font_path) {}

bool FontLoader::Init(const std::filesystem::path& executable_path) {
  if (auto font = resource::Find(executable_path, alphabet_font_path_); font) {
    alphabet_font_path_ = *font;
  } else {
    return false;
  }

  if (auto font = resource::Find(executable_path, symbols_font_path_); font) {
    symbols_font_path_ = *font;
  } else {
    return false;
  }

  ImGuiIO& imgui_io = ImGui::GetIO();

  if (ImFont* alphabet_font =
          imgui_io.Fonts->AddFontFromFileTTF(alphabet_font_path_.c_str());
      alphabet_font == nullptr) {
    return false;
  }

  ImFontConfig cfg;
  cfg.MergeMode = true;
  if (ImFont* symbol_font = imgui_io.Fonts->AddFontFromFileTTF(
          symbols_font_path_.c_str(), 0.0f, &cfg);
      symbol_font == nullptr) {
    return false;
  }

  return true;
}

void FontLoader::SetScale(float scale) {
  ImGui::GetStyle() = {};
  ImGui::GetStyle().ScaleAllSizes(scale);
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
