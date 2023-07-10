// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/utils/imgui_.h"

#include <cmath>
#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

#include <imgui.h>
#include <imgui_impl_sdlrenderer2.h>

#include "xpano/constants.h"
#include "xpano/utils/resource.h"

namespace xpano::utils::imgui {

FontLoader::FontLoader(std::string alphabet_font_path,
                       std::string symbols_font_path)
    : alphabet_font_path_(std::move(alphabet_font_path)),
      symbols_font_path_(std::move(symbols_font_path)) {}

void FontLoader::ComputeGlyphRanges() {
  ImFontGlyphRangesBuilder builder;
  builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesDefault());
  builder.BuildRanges(&alphabet_ranges_);

  builder.Clear();
  builder.AddText(kCheckMark);
  builder.AddText(kCommandSymbol);
  builder.BuildRanges(&symbol_ranges_);
}

bool FontLoader::Init(const std::filesystem::path& executable_path) {
  ComputeGlyphRanges();

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

  return true;
}

void FontLoader::Reload(float scale) {
  ImGuiIO& imgui_io = ImGui::GetIO();
  imgui_io.Fonts->Clear();
  imgui_io.Fonts->AddFontFromFileTTF(alphabet_font_path_.c_str(),
                                     std::roundf(18.0f * scale), nullptr,
                                     alphabet_ranges_.Data);
  ImFontConfig config;
  config.MergeMode = true;
  imgui_io.Fonts->AddFontFromFileTTF(symbols_font_path_.c_str(),
                                     std::roundf(18.0f * scale), &config,
                                     symbol_ranges_.Data);

  ImGui_ImplSDLRenderer2_DestroyDeviceObjects();
  ImGui_ImplSDLRenderer2_CreateDeviceObjects();

  ImGui::GetStyle() = {};
  ImGui::GetStyle().ScaleAllSizes(scale);
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
  int num_lines = static_cast<int>(lines.size());
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
