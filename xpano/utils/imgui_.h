#pragma once

#include <filesystem>
#include <optional>
#include <string>

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

}  // namespace xpano::utils::imgui
