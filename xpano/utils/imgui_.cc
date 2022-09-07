#include "xpano/utils/imgui_.h"

#include <cmath>
#include <filesystem>
#include <optional>
#include <utility>

#include <imgui.h>
#include <imgui_impl_sdlrenderer.h>

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

  ImGui_ImplSDLRenderer_DestroyDeviceObjects();
  ImGui_ImplSDLRenderer_CreateDeviceObjects();

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

}  // namespace xpano::utils::imgui
