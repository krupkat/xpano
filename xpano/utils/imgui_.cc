#include "imgui_.h"

#include <cmath>

#include <imgui.h>
#include <imgui_impl_sdlrenderer.h>

namespace xpano::utils::imgui {

void ReloadFont(const std::string& font_path, float scale) {
  ImGuiIO& imgui_io = ImGui::GetIO();

  imgui_io.Fonts->Clear();
  imgui_io.Fonts->AddFontFromFileTTF(font_path.c_str(),
                                     std::roundf(14.0f * scale));

  ImGui_ImplSDLRenderer_DestroyDeviceObjects();
  ImGui_ImplSDLRenderer_CreateDeviceObjects();

  ImGui::GetStyle() = {};
  ImGui::GetStyle().ScaleAllSizes(scale);
}

}  // namespace xpano::utils::imgui
