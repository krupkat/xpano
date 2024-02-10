// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <imgui.h>

namespace xpano::utils::imgui {

class FontLoader {
 public:
  FontLoader(std::string alphabet_font_path, std::string symbols_font_path);
  bool Init(const std::filesystem::path& executable_path);
  void Reload(float scale);

 private:
  void ComputeGlyphRanges();

  std::string alphabet_font_path_;
  std::string symbols_font_path_;
  ImVector<ImWchar> alphabet_ranges_;
  ImVector<ImWchar> symbol_ranges_;
};

void InfoMarker(const std::string& label, const std::string& desc);

std::string InitIniFilePath(std::optional<std::filesystem::path> app_data_path);

void DrawScrollableText(const char* label,
                        const std::vector<std::string>& lines,
                        ImVec2 size = ImVec2(0, 0));

template <typename TCallbackType>
void EnableIf(bool condition, TCallbackType callback,
              const char* disabled_label) {
  if (!condition) {
    ImGui::BeginDisabled();
  }
  callback();
  if (!condition) {
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
      ImGui::BeginTooltip();
      ImGui::TextUnformatted(disabled_label);
      ImGui::EndTooltip();
    }
  }
}

template <typename TOptionType, std::size_t N>
bool ComboBox(TOptionType* current_option,
              const std::array<TOptionType, N>& options, const char* label,
              ImGuiComboFlags flags = 0) {
  bool selected = false;
  if (ImGui::BeginCombo(label, Label(*current_option), flags)) {
    for (const auto option : options) {
      if (ImGui::Selectable(Label(option), option == *current_option)) {
        *current_option = option;
        selected = true;
      }
    }
    ImGui::EndCombo();
  }
  return selected;
}

template <typename TOptionType, std::size_t N>
void RadioBox(TOptionType* current_option,
              const std::array<TOptionType, N>& options) {
  for (const auto& option : options) {
    if (ImGui::RadioButton(Label(option), option == *current_option)) {
      *current_option = option;
    }
    ImGui::SameLine();
  }
}

ImVec2 DpiAwareSize(int width, int height);

}  // namespace xpano::utils::imgui
