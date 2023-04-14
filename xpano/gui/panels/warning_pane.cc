#include "xpano/gui/panels/warning_pane.h"

#include <imgui.h>
#include <spdlog/spdlog.h>

#include "xpano/constants.h"
#include "xpano/gui/action.h"

namespace xpano::gui {

namespace {
const char* WarningMessage(WarningType warning) {
  switch (warning) {
    case WarningType::kWarnInputConversion:
      return "Only 8-bit stitching pipeline is implemented!\nHigher bit depth "
             "images are converted to 8-bit.";
    case WarningType::kFirstTimeLaunch:
      return "Your friendly panorama stitching app:\n"
             " - default settings are designed to work out of the box with "
             "most images\n"
             " - hover over the little question marks for detailed "
             "instructions";
    case WarningType::kUserPrefBreakingChange:
      return "The user settings format has changed, reverting to defaults.";
    case WarningType::kUserPrefCouldntLoad:
      return "Couldn't load user settings, reverting to defaults.";
    case WarningType::kUserPrefResetOnRequest:
      return "User settings were reset to default values,\nyou can keep "
             "experimenting!";
    default:
      return "";
  }
}

const char* Title(WarningType warning) {
  switch (warning) {
    case WarningType::kFirstTimeLaunch:
      return "Welcome to Xpano!";
    case WarningType::kUserPrefResetOnRequest:
      return "Info";
    default:
      return "Warning!";
  }
}

bool EnableSnooze(WarningType warning) {
  switch (warning) {
    case WarningType::kWarnInputConversion:
      return true;
    default:
      return false;
  }
}
}  // namespace

void WarningPane::Draw() {
  if (current_warning_ == WarningType::kNone && !pending_warnings_.empty()) {
    Show(pending_warnings_.front());
    pending_warnings_.pop();
  }

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  if (ImGui::BeginPopupModal(Title(current_warning_), nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted(WarningMessage(current_warning_));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const auto text_base_width = ImGui::CalcTextSize("A").x;
    if (ImGui::Button("OK", ImVec2(text_base_width * kWideButtonWidth, 0))) {
      ImGui::CloseCurrentPopup();
      current_warning_ = WarningType::kNone;
    }
    if (EnableSnooze(current_warning_)) {
      ImGui::SameLine();
      bool dont_show_again = dont_show_again_.contains(current_warning_);
      if (ImGui::Checkbox("Do not warn next time", &dont_show_again)) {
        if (dont_show_again) {
          dont_show_again_.insert(current_warning_);
        } else {
          dont_show_again_.erase(current_warning_);
        }
      }
    }
    ImGui::EndPopup();
  }
}

void WarningPane::Queue(WarningType warning) {
  pending_warnings_.push(warning);
}

void WarningPane::Show(WarningType warning) {
  if (!dont_show_again_.contains(warning)) {
    ImGui::OpenPopup(Title(warning));
    current_warning_ = warning;
  }
  spdlog::warn(WarningMessage(current_warning_));
}

}  // namespace xpano::gui
