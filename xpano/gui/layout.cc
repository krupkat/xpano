#include "gui/layout.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "constants.h"

namespace xpano::gui {

namespace {
void InitDockSpace(ImGuiID dockspace_id, ImVec2 viewport_size) {
  const auto text_base_width = ImGui::CalcTextSize("A").x;
  const auto sidebar_width = kSidebarWidth * text_base_width;

  ImGuiID dock_main_id = dockspace_id;
  ImGuiID dock_id_thumbnails = ImGui::DockBuilderSplitNode(
      dock_main_id, ImGuiDir_Down, 0.20f, nullptr, &dock_main_id);
  ImGuiID dock_id_sidebar = ImGui::DockBuilderSplitNode(
      dock_main_id, ImGuiDir_Left, sidebar_width / viewport_size.x, nullptr,
      &dock_main_id);
  ImGuiID dock_id_logger = ImGui::DockBuilderSplitNode(
      dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);

  ImGui::DockBuilderDockWindow("PanoSweep", dock_id_sidebar);
  ImGui::DockBuilderDockWindow("Preview", dock_main_id);
  ImGui::DockBuilderDockWindow("Logger", dock_id_logger);
  ImGui::DockBuilderDockWindow("Images", dock_id_thumbnails);
  ImGui::DockBuilderFinish(dockspace_id);
}
}  // namespace

void Layout::Begin() {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
  ImGui::PopStyleVar(3);

  ImGuiID dockspace_id = ImGui::GetID("DockSpace");
  bool init_dockspace = ImGui::DockBuilderGetNode(dockspace_id) == nullptr;
  dockspace_id = ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f),
                                  ImGuiDockNodeFlags_NoTabBar);
  ImGui::End();

  if (init_dockspace) {
    InitDockSpace(dockspace_id, viewport->Size);
  }
}

void Layout::ToggleDebugInfo() { show_debug_info_ = !show_debug_info_; }

bool Layout::ShowDebugInfo() const { return show_debug_info_; }

}  // namespace xpano::gui
