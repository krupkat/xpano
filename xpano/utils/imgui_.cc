#include "imgui_.h"

#include <cmath>

#include <imgui.h>
#include <imgui_impl_sdlrenderer.h>

namespace xpano::utils::imgui {

void ReloadFont(const std::string& font_path, float scale) {
  ImGuiIO& imgui_io = ImGui::GetIO();

  imgui_io.Fonts->Clear();
  imgui_io.Fonts->AddFontFromFileTTF(font_path.c_str(),
                                     std::roundf(18.0f * scale), nullptr,
                                     imgui_io.Fonts->GetGlyphRangesDefault());

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
