#include "xpano/gui/panels/warning_pane.h"

#include <imgui.h>
#include <spdlog/spdlog.h>

#include "xpano/constants.h"
#include "xpano/gui/action.h"

namespace xpano::gui {

namespace {
const char* WarningMessage(ActionType action_type) {
  switch (action_type) {
    case ActionType::kWarnInputConversion:
      return "Only 8-bit stitching pipeline is implemented!\nHigher bit depth "
             "images are converted to 8-bit depth.";
    default:
      return "";
  }
}
}  // namespace

void WarningPane::Draw() {
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  if (ImGui::BeginPopupModal("Warning!", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text(WarningMessage(current_action_type_));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const auto text_base_width = ImGui::CalcTextSize("A").x;
    if (ImGui::Button("OK", ImVec2(text_base_width * kWideButtonWidth, 0))) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    bool dont_show_again = dont_show_again_.contains(current_action_type_);
    if (ImGui::Checkbox("Do not warn next time", &dont_show_again)) {
      if (dont_show_again) {
        dont_show_again_.insert(current_action_type_);
      } else {
        dont_show_again_.erase(current_action_type_);
      }
    }
    ImGui::EndPopup();
  }
}

void WarningPane::Show(Action action) {
  if (!dont_show_again_.contains(action.type)) {
    ImGui::OpenPopup("Warning!");
    current_action_type_ = action.type;
  }
  spdlog::warn(WarningMessage(current_action_type_));
}

}  // namespace xpano::gui
