#include "imgui_.h"

#include <cmath>

#include <imgui.h>
#include <imgui_impl_sdlrenderer.h>
#include <spdlog/spdlog.h>

#include "constants.h"

namespace xpano::utils::imgui {

FontLoader::FontLoader(std::string alphabet_font_path,
                       std::string symbols_font_path)
    : alphabet_font_path_(alphabet_font_path),
      symbols_font_path_(symbols_font_path) {}

void FontLoader::ComputeGlyphRanges() {
  ImFontGlyphRangesBuilder builder;
  builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesDefault());
  builder.BuildRanges(&alphabet_ranges_);

  builder.Clear();
  builder.AddText(kCheckMark);
  builder.BuildRanges(&symbol_ranges_);
}

void FontLoader::Reload(float scale) {
  if (alphabet_ranges_.empty() || symbol_ranges_.empty()) {
    ComputeGlyphRanges();
  }

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
  ImGui::TextDisabled(label.c_str());
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(desc.c_str());
    ImGui::EndTooltip();
  }
}

}  // namespace xpano::utils::imgui
