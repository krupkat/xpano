
#include "xpano/gui/panels/log_pane.h"

#include <imgui.h>

namespace xpano::gui {

LogPane::LogPane(logger::Logger *logger) : logger_(logger) {}

void LogPane::Draw() {
  if (!show_) {
    return;
  }

  ImGui::Begin("Logger");
  const auto &log = logger_->Log();
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  for (const auto &line : log) {
    ImGui::TextUnformatted(line.c_str());
  }
  ImGui::PopStyleVar();
  if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
    ImGui::SetScrollHereY(1.0f);
  }
  ImGui::End();
}

void LogPane::ToggleShow() { show_ = !show_; }

bool LogPane::IsShown() const { return show_; }

}  // namespace xpano::gui
